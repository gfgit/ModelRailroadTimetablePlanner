#include "jobstopsimporter.h"

#include "jobs/jobsmanager/model/jobshelper.h"
#include "jobs/jobeditor/model/stopmodel.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QDebug>

JobStopsImporter::JobStopsImporter(sqlite3pp::database &db) :
    mDb(db)
{

}

bool JobStopsImporter::createJob(db_id jobId, JobCategory category, const QVector<Stop> &stops)
{
    if(!JobsHelper::createNewJob(mDb, jobId, category))
        return false;

    StopModel model(mDb); //TODO: make member
    if(!model.loadJobStops(jobId))
    {
        JobsHelper::removeJob(mDb, jobId);
        return false;
    }

    sqlite3pp::query q_getStId(mDb, "SELECT id FROM stations WHERE UPPER(name) = ?1 OR UPPER(short_name) = ?1");
    sqlite3pp::query q_getPlatfId(mDb, "SELECT id FROM station_tracks WHERE station_id = ? AND UPPER(name) = ?");

    sqlite3pp::query q_getSeg(mDb, "SELECT s.id, s.in_gate_id, s.out_gate_id FROM railway_segments s"
                                   " JOIN station_gates g1 ON g1.id=s.in_gate_id"
                                   " JOIN station_gates g2 ON g2.id=s.out_gate_id"
                                   " WHERE (g1.station_id=?1 AND g2.station_id=?2)"
                                   " OR (g1.station_id=?2 AND g2.station_id=?1)");

    int i = 0;
    for(const Stop& row : stops)
    {
        //Get previous stop from model
        StopItem prevStop;
        if(i > 0)
            prevStop = model.getItemAt(i - 1);

        //Parse station
        q_getStId.bind(1, row.stationName.toUpper());
        if(q_getStId.step() != SQLITE_ROW)
        {
            q_getStId.reset();
            if(i == 0)
                continue; //Skip station at beginning of Job
            else
                break; //Truncate Job at last found station
        }

        db_id stationId = q_getStId.getRows().get<db_id>(0);
        q_getStId.reset();

        //Find segment between previous stop and current
        db_id prevSeg = 0;
        if(prevStop.stationId)
        {
            q_getSeg.bind(1, prevStop.stationId);
            q_getSeg.bind(2, stationId);
            int ret = q_getSeg.step();
            prevSeg = q_getSeg.getRows().get<db_id>(0);
            if(ret != SQLITE_ROW || !prevSeg)
            {
                qDebug() << "Error Job:" << jobId << "Stations:" << prevStop.stationId << "and" << stationId << "not connected";
                qDebug() << mDb.error_msg();
                break;
            }
            q_getSeg.reset();
        }

        //Create new (current) stop
        model.addStop();

        //Set next segment of previous stop (now is not Last anymore)
        db_id outSegGateId = 0;
        if(prevSeg)
        {
            if(model.trySelectNextSegment(prevStop, prevSeg, 0, stationId, outSegGateId))
            {
                StopItem::Segment beforePrevSeg;
                if(i > 1)
                    beforePrevSeg = model.getItemAt(i - 2).nextSegment;

                //Store changes to previous stop before editing current one
                model.setStopInfo(model.index(i - 1), prevStop, beforePrevSeg);
            }
        }

        //Get stop after saving previous
        StopItem curStop = model.getItemAt(i);
        curStop.stationId = stationId;
        curStop.arrival = row.arrival;
        curStop.departure = row.departure;

        model.trySelectTrackForStop(curStop);

        db_id newTrackId = 0;
        if(!row.platformName.isEmpty())
        {
            q_getPlatfId.bind(1, curStop.stationId);
            q_getPlatfId.bind(2, row.platformName.toUpper());
            q_getPlatfId.step();
            newTrackId = q_getPlatfId.getRows().get<db_id>(0);
            q_getPlatfId.reset();
        }

        if(newTrackId)
        {
            QString errMsg;
            if(!model.trySetTrackConnections(curStop, newTrackId, &errMsg))
            {
                qDebug() << "Platf error:" << curStop.stationId << newTrackId << errMsg;
            }
        }

        model.setStopInfo(model.index(i), curStop, prevStop.nextSegment);

        i++;
    }

    model.commitChanges();

    if(model.rowCount() < 3)
    {
        //A Job must have at least 2 stop (origin, destination)
        //Plus 1 row "Add Here" added by the model
        JobsHelper::removeJob(mDb, model.getJobId());
        return false;
    }

    return true;
}
