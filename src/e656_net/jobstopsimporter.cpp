#include "jobstopsimporter.h"

#include "jobs/jobsmanager/model/jobshelper.h"
#include "jobs/jobeditor/model/stopmodel.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QDebug>

JobStopsImporter::JobStopsImporter(sqlite3pp::database &db) :
    mDb(db)
{

}

bool createStop(StopModel &model, const JobStopsImporter::Stop& stopInfo,
                db_id stationId, db_id prevSegId, db_id trackId,
                QTime &prevDeparture, int i)
{

    //Create new (current) stop
    model.addStop();

    //Get previous stop from model
    StopItem prevStop;
    if(i > 0)
    {
        prevStop = model.getItemAt(i - 1);
    }

    //Set next segment of previous stop (now is not Last anymore)
    if(prevSegId)
    {
        db_id outSegGateId = 0;
        db_id suggestedTrackId = 0;
        if(!model.trySelectNextSegment(prevStop, prevSegId, 0, stationId, outSegGateId, suggestedTrackId))
        {
            //Maybe selected track is not connected to out gate
            if(suggestedTrackId > 0)
            {
                //Try again with suggested track
                model.trySetTrackConnections(prevStop, suggestedTrackId, nullptr);
                model.trySelectNextSegment(prevStop, prevSegId, 0, stationId, outSegGateId, suggestedTrackId);
            }
        }
    }

    if(i > 0)
    {
        //Apply departure to previous stop now that it's not Last anymore
        prevStop.departure = prevDeparture;

        StopItem::Segment beforePrevSeg;
        if(i > 1)
            beforePrevSeg = model.getItemAt(i - 2).nextSegment;

        //Store changes to previous stop before editing current one
        model.setStopInfo(model.index(i - 1), prevStop, beforePrevSeg);
        prevStop = model.getItemAt(i - 1);
    }

    //Get stop after saving previous
    StopItem curStop = model.getItemAt(i);
    curStop.stationId = stationId;
    curStop.arrival = stopInfo.arrival;
    curStop.departure = stopInfo.departure;

    //Cannot set departure directly (unless it's First stop) because we are Last stop
    //On next stop we will go back and set departure
    prevDeparture = curStop.departure;

    model.trySelectTrackForStop(curStop);

    if(trackId)
    {
        QString errMsg;
        if(!model.trySetTrackConnections(curStop, trackId, &errMsg))
        {
            qDebug() << "Platf error:" << curStop.stationId << trackId << errMsg;
        }
    }

    model.setStopInfo(model.index(i), curStop, prevStop.nextSegment);

    return true;
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

    sqlite3pp::query q_findLine(mDb,
                                "SELECT lines.id, l1.pos, l1.direction, l2.pos, l2.direction, lines.name, g1_in.name, g1_out.name,"
                                "MIN(ABS(l1.pos - l2.pos))"
                                " FROM lines"
                                " JOIN line_segments l1 ON lines.id=l1.line_id"
                                " JOIN line_segments l2 ON lines.id=l2.line_id"
                                " JOIN railway_segments s1 ON s1.id=l1.seg_id"
                                " JOIN railway_segments s2 ON s2.id=l2.seg_id"
                                " JOIN station_gates g1_in ON g1_in.id=s1.in_gate_id"
                                " JOIN station_gates g1_out ON g1_out.id=s1.out_gate_id"
                                " JOIN station_gates g2_in ON g2_in.id=s2.in_gate_id"
                                " JOIN station_gates g2_out ON g2_out.id=s2.out_gate_id"
                                " WHERE (g1_out.station_id=?1 OR g1_in.station_id=?1)"
                                " AND (g2_out.station_id=?2 OR g2_in.station_id=?2)");

    sqlite3pp::query q_getLineSeg(mDb,
                                  "SELECT line_seg.seg_id, line_seg.direction, g_in.station_id, g_out.station_id"
                                  " FROM line_segments line_seg"
                                  " JOIN railway_segments seg ON seg.id=line_seg.seg_id"
                                  " JOIN station_gates g_in ON g_in.id=seg.in_gate_id"
                                  " JOIN station_gates g_out ON g_out.id=seg.out_gate_id"
                                  " WHERE line_seg.line_id=?1 AND line_seg.pos BETWEEN ?2 AND ?3"
                                  " ORDER BY line_seg.pos ASC");

    db_id prevStationId = 0;
    QTime prevDeparture;
    int i = 0;
    int ret = 0;

    for(const Stop& row : stops)
    {
        //Parse station
        q_getStId.bind(1, row.stationName.toUpper());
        ret = q_getStId.step();
        db_id stationId = q_getStId.getRows().get<db_id>(0);
        q_getStId.reset();

        if(ret != SQLITE_ROW)
        {
            qDebug().quote() << "Could not find station:" << row.stationName;
            if(i == 0)
                continue; //Skip station at beginning of Job
            else
                break; //Truncate Job at last found station
        }

        //Find segment between previous stop and current
        db_id prevSegId = 0;
        bool needsLineSearch = false;

        if(prevStationId)
        {
            q_getSeg.bind(1, prevStationId);
            q_getSeg.bind(2, stationId);
            ret = q_getSeg.step();
            prevSegId = q_getSeg.getRows().get<db_id>(0);
            q_getSeg.reset();

            if(ret == SQLITE_DONE || ret == SQLITE_OK)
            {
                needsLineSearch = true;
            }
            else if(ret != SQLITE_ROW || !prevSegId)
            {
                qDebug() << "Error Job:" << jobId << "Stations:" << prevStationId << "and" << stationId << "not connected";
                qDebug() << mDb.error_msg();
                break;
            }
        }

        if(needsLineSearch)
        {
            //Try a line connecting desired stations
            q_findLine.bind(1, prevStationId);
            q_findLine.bind(2, stationId);
            ret = q_findLine.step();
            if(ret != SQLITE_ROW)
            {
                qDebug() << "Cannot find line between stations" << stationId << prevStationId << ret << mDb.error_code();
                q_findLine.reset();
                break;
            }

            auto line = q_findLine.getRows();
            db_id lineId = line.get<db_id>(0);
            int fromSegPos = line.get<int>(1);
            int toSegPos = line.get<int>(3);
            q_findLine.reset();

            bool invertSegOrder = false;
            if(fromSegPos > toSegPos)
            {
                qSwap(fromSegPos, toSegPos);
                invertSegOrder = true;
            }

            qDebug() << "FOUND LINE:" << line.get<QString>(4);

            q_getLineSeg.bind(1, lineId);
            q_getLineSeg.bind(2, fromSegPos);
            q_getLineSeg.bind(3, toSegPos);

            struct SegInfo
            {
                db_id segmentId = 0;
                db_id toStationId = 0;
            };

            QVector<SegInfo> segInfoVec(toSegPos - fromSegPos + 1);

            int j = 0;
            for(auto seg : q_getLineSeg)
            {
                SegInfo info;
                info.segmentId = seg.get<db_id>(0);
                bool inverted = seg.get<int>(1) != 0;
                db_id fromStId = seg.get<db_id>(2);
                info.toStationId = seg.get<db_id>(3);

                if(inverted != invertSegOrder)
                    qSwap(fromStId, info.toStationId);

                if(invertSegOrder)
                    segInfoVec[segInfoVec.size() - j - 1] = info; //Reverse order
                else
                    segInfoVec[j] = info;
                j++;
            }
            q_getLineSeg.reset();

            if(j != segInfoVec.size())
                break; //Error getting segments

            for(const SegInfo seg : qAsConst(segInfoVec))
            {
                JobStopsImporter::Stop stopInfo;
                stopInfo.arrival = prevDeparture.addSecs(60);
                stopInfo.departure = stopInfo.arrival;

                if(seg.toStationId == stationId)
                {
                    //Add last one as normal stop
                    prevSegId = seg.segmentId;
                    break;
                }

                createStop(model, stopInfo, seg.toStationId, seg.segmentId, 0, prevDeparture, i);
                prevStationId = seg.toStationId;
                i++;
            }
        }

        db_id newTrackId = 0;
        if(!row.platformName.isEmpty())
        {
            q_getPlatfId.bind(1, stationId);
            q_getPlatfId.bind(2, row.platformName.toUpper());
            q_getPlatfId.step();
            newTrackId = q_getPlatfId.getRows().get<db_id>(0);
            q_getPlatfId.reset();
        }

        createStop(model, row, stationId, prevSegId, newTrackId, prevDeparture, i);
        prevStationId = stationId;
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
