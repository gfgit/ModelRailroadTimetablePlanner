#include "railwaysegmenthelper.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

RailwaySegmentHelper::RailwaySegmentHelper(sqlite3pp::database &db) :
    mDb(db)
{

}

bool RailwaySegmentHelper::getSegmentInfo(db_id segmentId,
                                          QString &outName, utils::RailwaySegmentType &outType,
                                          int &outDistance, int &outSpeed,
                                          db_id &outFromStId, db_id &outFromGateId,
                                          db_id &outToStId, db_id &outToGateId)
{
    query q(mDb, "SELECT s.in_gate_id,s.out_gate_id,s.name,s.max_speed_kmh,"
                 "s.type,s.distance_meters,"
                 "g1.station_id,g2.station_id"
                 " FROM railway_segments s"
                 " JOIN station_gates g1 ON g1.id=s.in_gate_id"
                 " JOIN station_gates g2 ON g2.id=s.out_gate_id"
                 " WHERE s.id=?");
    q.bind(1, segmentId);
    if(q.step() != SQLITE_ROW)
        return false; //FIXME: show error to user

    auto r = q.getRows();
    outFromGateId = r.get<db_id>(0);
    outToGateId = r.get<db_id>(1);
    outName = r.get<QString>(2);
    outSpeed = r.get<int>(3);
    outType = utils::RailwaySegmentType(r.get<db_id>(4));
    outDistance = r.get<int>(5);
    outFromStId = r.get<db_id>(6);
    outToStId = r.get<db_id>(7);

    return true;
}

bool RailwaySegmentHelper::setSegmentInfo(db_id &segmentId, bool create,
                                          const QString &name, utils::RailwaySegmentType type,
                                          int distance, int speed,
                                          db_id fromGateId, db_id toGateId,
                                          QString *outErrMsg)
{
    command cmd(mDb);
    if(create)
    {
        cmd.prepare("INSERT INTO railway_segments"
                    "(id, in_gate_id, out_gate_id, name, max_speed_kmh, type, distance_meters)"
                    " VALUES(NULL,?1,?2,?3,?4,?5,?6)");
    }else{
        cmd.prepare("UPDATE railway_segments SET"
                    " in_gate_id=?1, out_gate_id=?2, name=?3, max_speed_kmh=?4, type=?5, distance_meters=?6"
                    " WHERE id=?7");
        cmd.bind(7, segmentId);
    }

    cmd.bind(1, fromGateId);
    cmd.bind(2, toGateId);
    cmd.bind(3, name);
    cmd.bind(4, speed);
    cmd.bind(5, int(type));
    cmd.bind(6, distance);

    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        if(outErrMsg)
            *outErrMsg = mDb.error_msg();
        return false; //FIXME: show error to user
    }

    if(create)
        segmentId = mDb.last_insert_rowid();

    return true;
}

bool RailwaySegmentHelper::removeSegment(db_id segmentId, QString *outErrMsg)
{
    command cmd(mDb, "DELETE FROM railway_segments WHERE id=?");
    cmd.bind(1, segmentId);
    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        if(outErrMsg)
            *outErrMsg = mDb.error_msg();
        return false;
    }
    return true;
}
