#include "linegraphselectionhelper.h"

#include "linegraphscene.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

LineGraphSelectionHelper::LineGraphSelectionHelper(sqlite3pp::database &db) :
    mDb(db)
{

}

bool LineGraphSelectionHelper::tryFindJobStopInGraph(LineGraphScene *scene, db_id jobId, SegmentInfo &info)
{
    query q(mDb);

    switch (scene->getGraphType())
    {
    case LineGraphType::SingleStation:
    {
        q.prepare("SELECT stops.id, c.seg_id, MIN(stops.arrival), stops.departure"
                  " FROM stops"
                  " LEFT JOIN railway_connections c ON c.id=stops.next_segment_conn_id"
                  " WHERE stops.job_id=? AND stops.station_id=?");
        break;
    }
    case LineGraphType::RailwaySegment:
    {
        q.prepare("SELECT stops.id, c.seg_id, MIN(stops.arrival), stops.departure, stops.station_id"
                  " FROM railway_segments seg"
                  " JOIN station_gates g1 ON g1.id=seg.in_gate_id"
                  " JOIN station_gates g2 ON g2.id=seg.out_gate_id"
                  " JOIN stops ON stops.job_id=? AND"
                  " (stops.station_id=g1.station_id OR stops.station_id=g2.station_id)"
                  " LEFT JOIN railway_connections c ON c.id=stops.next_segment_conn_id"
                  " WHERE seg.id=?");
        break;
    }
    case LineGraphType::RailwayLine:
    {
        q.prepare("SELECT stops.id, c.seg_id, MIN(stops.arrival), stops.departure, stops.station_id"
                  " FROM line_segments"
                  " JOIN railway_segments seg ON seg.id=line_segments.seg_id"
                  " JOIN station_gates g1 ON g1.id=seg.in_gate_id"
                  " JOIN station_gates g2 ON g2.id=seg.out_gate_id"
                  " JOIN stops ON stops.job_id=? AND"
                  " (stops.station_id=g1.station_id OR stops.station_id=g2.station_id)"
                  " LEFT JOIN railway_connections c ON c.id=stops.next_segment_conn_id"
                  " WHERE line_segments.line_id=?");
        break;
    }
    case LineGraphType::NoGraph:
    case LineGraphType::NTypes:
    {
        //We need to load a new graph, give up
        return false;
    }
    }

    q.bind(1, jobId);
    q.bind(2, scene->getGraphObjectId());

    if(q.step() != SQLITE_ROW || q.getRows().column_type(0) == SQLITE_NULL)
        return false; //We didn't find a stop in current graph, give up

    //Get stop info
    auto stop = q.getRows();
    info.firstStopId = stop.get<db_id>(0);
    info.segmentId = stop.get<db_id>(1);
    info.arrivalAndStart = stop.get<QTime>(2);
    info.departure = stop.get<QTime>(3);

    //If graph is SingleStation we already know the station ID
    if(scene->getGraphType() == LineGraphType::SingleStation)
        info.firstStationId = scene->getGraphObjectId();
    else
        info.firstStationId = stop.get<db_id>(4);

    return true;
}

bool LineGraphSelectionHelper::tryFindJobStopsAfter(db_id jobId, SegmentInfo& info)
{
    query q(mDb, "SELECT stops.id, MIN(stops.arrival), stops.departure, stops.station_id, c.seg_id"
                 " FROM stops"
                 " LEFT JOIN railway_connections c ON c.id=stops.next_segment_conn_id"
                 " WHERE stops.job_id=? AND stops.arrival>=?");

    //Find first job stop after or equal to startTime
    q.bind(1, jobId);
    q.bind(2, info.arrivalAndStart);
    if(q.step() != SQLITE_ROW || q.getRows().column_type(0) == SQLITE_NULL)
        return false;

    //Get first stop info
    auto stop = q.getRows();
    info.firstStopId = stop.get<db_id>(0);
    info.arrivalAndStart = stop.get<QTime>(1);
    info.departure = stop.get<QTime>(2);
    info.firstStationId = stop.get<db_id>(3);
    info.segmentId = stop.get<db_id>(4);
    q.reset();

    //Try get a second stop after the first departure
    //NOTE: minimum 60 seconds of travel between 2 consecutive stops
    q.bind(1, jobId);
    q.bind(2, info.departure.addSecs(60));
    if(q.step() != SQLITE_ROW)
    {
        //We found only 1 stop, return that
        info.secondStationId = 0;
        return true;
    }

    //Get first stop info
    stop = q.getRows();
    //db_id secondStopId = stop.get<db_id>(0);
    //QTime secondArrival = stop.get<QTime>(1);
    info.departure = stop.get<QTime>(2); //Overwrite departure
    info.secondStationId = stop.get<db_id>(3);

    return true;
}

bool LineGraphSelectionHelper::tryFindNewGraphForJob(db_id jobId, SegmentInfo& info,
                                                     db_id &outGraphObjId, LineGraphType &outGraphType)
{
    if(!tryFindJobStopsAfter(jobId, info))
        return false; //No stops found

    if(!info.secondStationId || !info.segmentId)
    {
        //We found only 1 stop, select first station
        outGraphObjId = info.firstStationId;
        outGraphType = LineGraphType::SingleStation;
        return true;
    }

    //Try to find a railway line which contains this segment
    //FIXME: better criteria to choose a line (name?, number of segments?, prompt user?)
    query q(mDb, "SELECT ls.line_id"
                 " FROM line_segments ls"
                 " WHERE ls.seg_id=?");
    q.bind(1, info.segmentId);

    if(q.step() == SQLITE_ROW)
    {
        //Found a line
        outGraphObjId = q.getRows().get<db_id>(0);
        outGraphType = LineGraphType::RailwayLine;
        return true;
    }

    //No lines found, use the railway segment
    outGraphObjId = info.segmentId;
    outGraphType = LineGraphType::RailwaySegment;

    return true;
}

bool LineGraphSelectionHelper::requestJobSelection(LineGraphScene *scene, db_id jobId, bool select, bool ensureVisible)
{
    //Check jobId is valid and get category

    query q(mDb, "SELECT category FROM jobs WHERE id=?");
    q.bind(1, jobId);
    if(q.step() != SQLITE_ROW)
        return false; //Job doen't exist

    JobStopEntry selectedJob;
    selectedJob.jobId = jobId;
    selectedJob.category = JobCategory(q.getRows().get<int>(0));

    SegmentInfo info;

    // Try to select earliest stop of this job in current graph, if any
    const bool found = tryFindJobStopInGraph(scene, selectedJob.jobId, info);

    if(!found)
    {
        //Find a NEW line graph or segment or station with this job

        //Find first 2 stops of the job

        db_id graphObjId = 0;
        LineGraphType graphType = LineGraphType::NoGraph;

        if(!tryFindNewGraphForJob(selectedJob.jobId, info, graphObjId, graphType))
        {
            //Could not find a suitable graph, abort
            return false;
        }

        //NOTE: clear selection to avoid LineGraphManager trying to follow selection
        //do not emit change because selection might be synced between all scenes
        //and because it's restored soon after
        const JobStopEntry oldSelection = scene->getSelectedJob();
        scene->setSelectedJob(JobStopEntry{}, false); //Clear selection

        //Select the graph
        scene->loadGraph(graphObjId, graphType);

        //Restore previous selection
        scene->setSelectedJob(oldSelection, false);
    }

    //Extract the info
    selectedJob.stopId = info.firstStopId;

    if(!selectedJob.stopId)
        return false; //No stop found, abort

    //Select job
    if(select)
        scene->setSelectedJob(selectedJob);

    if(ensureVisible)
    {
        return scene->requestShowZone(info.firstStationId, info.segmentId,
                                      info.arrivalAndStart, info.departure);
    }

    return true;
}

bool LineGraphSelectionHelper::requestCurrentJobPrevSegmentVisible(LineGraphScene *scene, bool goToStart)
{
    JobStopEntry selectedJob = scene->getSelectedJob();
    if(!selectedJob.jobId)
        return false; //No job selected, nothing to do

    query q(mDb);

    SegmentInfo info;
    if(selectedJob.stopId && !goToStart)
    {
        //Start from current stop and get previous stop
        q.prepare("SELECT s2.job_id, s1.id, MAX(s1.arrival), s1.station_id, c.seg_id,"
                  "s2.departure, s2.station_id"
                  " FROM stops s2"
                  " JOIN stops s1 ON s1.job_id=s2.job_id"
                  " LEFT JOIN railway_connections c ON c.id=s1.next_segment_conn_id"
                  " WHERE s2.id=? AND s1.arrival < s2.arrival");
        q.bind(1, selectedJob.stopId);

        if(q.step() == SQLITE_ROW && q.getRows().column_type(0) != SQLITE_NULL)
        {
            auto stop = q.getRows();
            db_id jobId = stop.get<db_id>(0);
            if(jobId == selectedJob.jobId)
            {
                //Found stop and belongs to requested job
                info.firstStopId = stop.get<db_id>(1);
                info.arrivalAndStart = stop.get<QTime>(2);
                info.firstStationId = stop.get<db_id>(3);
                info.segmentId = stop.get<db_id>(4);
                info.departure = stop.get<QTime>(5);
                info.secondStationId = stop.get<db_id>(6);
            }
        }
    }

    if(!info.firstStopId)
    {
        //goToStart or failed to get previous stop so go to start anyway

        if(!tryFindJobStopsAfter(selectedJob.jobId, info))
            return false; //No stops found, give up
    }

    db_id graphObjId = 0;
    LineGraphType graphType = LineGraphType::NoGraph;

    if(!tryFindNewGraphForJob(selectedJob.jobId, info, graphObjId, graphType))
    {
        //Could not find a suitable graph, abort
        return false;
    }

    //NOTE: clear selection to avoid LineGraphManager trying to follow selection
    //do not emit change because selection might be synced between all scenes
    //and because it's restored soon after
    scene->setSelectedJob(JobStopEntry{}, false); //Clear selection

    //Select the graph
    scene->loadGraph(graphObjId, graphType);

    //Restore selection
    selectedJob.stopId = info.firstStopId;
    scene->setSelectedJob(selectedJob); //This time emit

    return scene->requestShowZone(info.firstStationId, info.segmentId,
                                  info.arrivalAndStart, info.departure);
}

bool LineGraphSelectionHelper::requestCurrentJobNextSegmentVisible(LineGraphScene *scene, bool goToEnd)
{
    //TODO: maybe go to first segment AFTER last segment in current view.
    //So it may not be 'next' but maybe 2 or 3 segments after current.

    JobStopEntry selectedJob = scene->getSelectedJob();
    if(!selectedJob.jobId)
        return false; //No job selected, nothing to do

    query q(mDb);

    SegmentInfo info;
    if(selectedJob.stopId && !goToEnd)
    {
        //Start from current stop and get previous stop
        q.prepare("SELECT s2.job_id, s1.id, MIN(s1.arrival), s1.departure, s1.station_id, c.seg_id"
                  " FROM stops s2"
                  " JOIN stops s1 ON s1.job_id=s2.job_id"
                  " LEFT JOIN railway_connections c ON c.id=s1.next_segment_conn_id"
                  " WHERE s2.id=? AND s1.arrival > s2.arrival");
        q.bind(1, selectedJob.stopId);

        if(q.step() == SQLITE_ROW && q.getRows().column_type(0) != SQLITE_NULL)
        {
            auto stop = q.getRows();
            db_id jobId = stop.get<db_id>(0);
            if(jobId == selectedJob.jobId)
            {
                //Found stop and belongs to requested job
                info.firstStopId = stop.get<db_id>(1);
                info.arrivalAndStart = stop.get<QTime>(2);
                info.departure = stop.get<QTime>(3);
                info.firstStationId = stop.get<db_id>(4);
                info.segmentId = stop.get<db_id>(5);
            }
        }
    }

    if(!info.firstStopId)
    {
        //goToEnd or failed to get next stop so go to end anyway

        //Select last station, no segment
        q.prepare("SELECT s1.id, MAX(s1.arrival), s1.departure, s1.station_id"
                  " FROM stops s1"
                  " WHERE s1.job_id=?");
        q.bind(1, selectedJob.jobId);

        if(q.step() == SQLITE_ROW && q.getRows().column_type(0) != SQLITE_NULL)
        {
            auto stop = q.getRows();
            db_id jobId = stop.get<db_id>(0);
            if(jobId == selectedJob.jobId)
            {
                //Found stop and belongs to requested job
                info.firstStopId = stop.get<db_id>(1);
                info.arrivalAndStart = stop.get<QTime>(2);
                info.departure = stop.get<QTime>(3);
                info.firstStationId = stop.get<db_id>(4);
            }
        }
    }

    db_id graphObjId = 0;
    LineGraphType graphType = LineGraphType::NoGraph;

    if(!tryFindNewGraphForJob(selectedJob.jobId, info, graphObjId, graphType))
    {
        //Could not find a suitable graph, abort
        return false;
    }

    //NOTE: clear selection to avoid LineGraphManager trying to follow selection
    //do not emit change because selection might be synced between all scenes
    //and because it's restored soon after
    scene->setSelectedJob(JobStopEntry{}, false); //Clear selection

    //Select the graph
    scene->loadGraph(graphObjId, graphType);

    //Restore selection
    selectedJob.stopId = info.firstStopId;
    scene->setSelectedJob(selectedJob); //This time emit

    return scene->requestShowZone(info.firstStationId, info.segmentId,
                                  info.arrivalAndStart, info.departure);
}
