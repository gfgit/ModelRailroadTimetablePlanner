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
        q.prepare("SELECT stops.id, MIN(stops.arrival), stops.departure"
                  " FROM stops"
                  " WHERE stops.job_id=? AND stops.station_id=?");
        break;
    }
    case LineGraphType::RailwaySegment:
    {
        q.prepare("SELECT stops.id, MIN(stops.arrival), stops.departure, stops.station_id"
                  " FROM railway_segments seg"
                  " JOIN station_gates g1 ON g1.id=seg.in_gate_id"
                  " JOIN station_gates g2 ON g2.id=seg.in_gate_id"
                  " JOIN stops ON stops.jobId=? AND"
                  " (stops.station_id=g1.station_id OR stops.station_id=g2.station_id)"
                  " WHERE seg.id=?");
        break;
    }
    case LineGraphType::RailwayLine:
    {
        q.prepare("SELECT stops.id, MIN(stops.arrival), stops.departure, stops.station_id"
                  " FROM line_segments"
                  " JOIN railway_segments seg ON seg.id=line_segments.seg_id"
                  " JOIN station_gates g1 ON g1.id=seg.in_gate_id"
                  " JOIN station_gates g2 ON g2.id=seg.in_gate_id"
                  " JOIN stops ON stops.jobId=? AND"
                  " (stops.station_id=g1.station_id OR stops.station_id=g2.station_id)"
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

    if(q.step() != SQLITE_ROW)
        return false; //We didn't find a stop in current graph, give up

    //Get stop info
    auto stop = q.getRows();
    info.firstStopId = stop.get<db_id>(0);
    info.arrivalAndStart = stop.get<QTime>(1);
    info.departure = stop.get<QTime>(2);

    //If graph is SingleStation we already know the station ID
    if(scene->getGraphType() == LineGraphType::SingleStation)
        info.firstStId = scene->getGraphObjectId();
    else
        info.firstStId = stop.get<db_id>(3);

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
    if(q.step() != SQLITE_ROW)
        return false;

    //Get first stop info
    auto stop = q.getRows();
    info.firstStopId = stop.get<db_id>(0);
    info.arrivalAndStart = stop.get<QTime>(1);
    info.departure = stop.get<QTime>(2);
    info.firstStId = stop.get<db_id>(3);
    info.segmentId = stop.get<db_id>(4);
    q.reset();

    //Try get a second stop after the first departure
    //NOTE: minimum 60 seconds of travel between 2 consecutive stops
    q.bind(1, jobId);
    q.bind(2, info.departure.addSecs(60));
    if(q.step() != SQLITE_ROW)
    {
        //We found only 1 stop, return that
        info.secondStId = 0;
        return true;
    }

    //Get first stop info
    stop = q.getRows();
    //db_id secondStopId = stop.get<db_id>(0);
    //QTime secondArrival = stop.get<QTime>(1);
    info.departure = stop.get<QTime>(2); //Overwrite departure
    info.secondStId = stop.get<db_id>(3);

    return true;
}

bool LineGraphSelectionHelper::tryFindNewGraphForJob(db_id jobId, SegmentInfo& info,
                                                     db_id &outGraphObjId, LineGraphType &outGraphType)
{
    if(!tryFindJobStopsAfter(jobId, info))
        return false; //No stops found

    if(!info.secondStId || !info.segmentId)
    {
        //We found only 1 stop, select first station
        outGraphObjId = info.firstStId;
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

        //Select the graph
        scene->loadGraph(graphObjId, graphType);
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
        return scene->requestShowZone(info.firstStId, info.segmentId,
                                      info.arrivalAndStart, info.departure);
    }

    return true;
}
