#include "railwaysegmenthelper.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QCoreApplication>

//Error messages
class RailwaySegmentHelperStrings
{
    Q_DECLARE_TR_FUNCTIONS(RailwaySegmentHelperStrings)
};

static constexpr char
    errorSegmentInUseText[] =
    QT_TRANSLATE_NOOP("RailwaySegmentHelperStrings",
                      "Cannot delete segment <b>%1</b> because it is still referenced.<br>"
                      "Please delete all jobs travelling on this segment or change their path.");

RailwaySegmentHelper::RailwaySegmentHelper(sqlite3pp::database &db) :
    mDb(db)
{

}

bool RailwaySegmentHelper::getSegmentInfo(SegmentInfo &info)
{
    query q(mDb, "SELECT s.name,s.max_speed_kmh,s.type,s.distance_meters,"
                 "s.in_gate_id,g1.station_id,"
                 "s.out_gate_id,g2.station_id"
                 " FROM railway_segments s"
                 " JOIN station_gates g1 ON g1.id=s.in_gate_id"
                 " JOIN station_gates g2 ON g2.id=s.out_gate_id"
                 " WHERE s.id=?");
    q.bind(1, info.segmentId);
    if(q.step() != SQLITE_ROW)
        return false; //FIXME: show error to user

    auto r = q.getRows();
    info.segmentName = r.get<QString>(0);
    info.maxSpeedKmH = r.get<int>(1);
    info.type = utils::RailwaySegmentType(r.get<db_id>(2));
    info.distanceMeters = r.get<db_id>(3);

    info.from.gateId = r.get<db_id>(4);
    info.from.stationId = r.get<db_id>(5);

    info.to.gateId = r.get<db_id>(6);
    info.to.stationId = r.get<db_id>(7);

    return true;
}

bool RailwaySegmentHelper::getSegmentInfoFromGate(db_id gateId, SegmentInfo &info)
{
    query q(mDb, "SELECT s.id,s.name,s.max_speed_kmh,s.type,s.distance_meters,"
                 "s.in_gate_id,g1.name,g1.station_id,st1.name,"
                 "s.out_gate_id,g2.name,g2.station_id,st2.name"
                 " FROM railway_segments s"
                 " JOIN station_gates g1 ON g1.id=s.in_gate_id"
                 " JOIN station_gates g2 ON g2.id=s.out_gate_id"
                 " JOIN stations st1 ON st1.id=g1.station_id"
                 " JOIN stations st2 ON st2.id=g2.station_id"
                 " WHERE g1.id=?1 OR g2.id=?1");
    q.bind(1, gateId);
    if(q.step() != SQLITE_ROW)
        return false; //FIXME: show error to user

    auto r = q.getRows();
    info.segmentId = r.get<db_id>(0);
    info.segmentName = r.get<QString>(1);
    info.maxSpeedKmH = r.get<int>(2);
    info.type = utils::RailwaySegmentType(r.get<db_id>(3));
    info.distanceMeters = r.get<db_id>(4);

    info.from.gateId = r.get<db_id>(5);
    info.from.gateLetter = r.get<const char*>(6)[0];
    info.from.stationId = r.get<db_id>(7);
    info.from.stationName = r.get<QString>(8);

    info.to.gateId = r.get<db_id>(9);
    info.to.gateLetter = r.get<const char*>(10)[0];
    info.to.stationId = r.get<db_id>(11);
    info.to.stationName = r.get<QString>(12);

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
    {
        segmentId = mDb.last_insert_rowid();
        emit Session->segmentAdded(segmentId);
    }

    return true;
}

bool RailwaySegmentHelper::removeSegment(db_id segmentId, QString *outErrMsg)
{
    //Use transaction so if one query fails, the other is restored
    sqlite3pp::transaction t(mDb);

    //First remove all connections belonging to this segments
    command cmd(mDb, "DELETE FROM railway_connections WHERE seg_id=?");
    cmd.bind(1, segmentId);
    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        if(outErrMsg)
        {
            if(ret == SQLITE_CONSTRAINT_FOREIGNKEY || ret == SQLITE_CONSTRAINT_TRIGGER)
            {
                //Get name
                query q(mDb, "SELECT name FROM railway_segments WHERE id=?");
                q.bind(1, segmentId);
                q.step();
                QString segName = q.getRows().get<QString>(0);
                q.finish();

                *outErrMsg = RailwaySegmentHelperStrings::tr(errorSegmentInUseText).arg(segName.toHtmlEscaped());
            }
            else
            {
                *outErrMsg = mDb.error_msg();
            }
        }
        return false;
    }

    //Then remove actual segment
    cmd.prepare("DELETE FROM railway_segments WHERE id=?");
    cmd.bind(1, segmentId);
    ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        if(outErrMsg)
            *outErrMsg = mDb.error_msg();
        return false;
    }

    t.commit();

    emit Session->segmentRemoved(segmentId);

    return true;
}

bool RailwaySegmentHelper::findFirstLineOrSegment(db_id &graphObjId, bool &isLine)
{
    //Try to find first Railway Line
    query q(mDb, "SELECT id, MIN(name) FROM lines");
    if(q.step() == SQLITE_ROW && q.getRows().column_type(0) != SQLITE_NULL)
    {
        isLine = true;
        graphObjId = q.getRows().get<db_id>(0);
        return true;
    }

    //Try with Railway Segment
    q.prepare("SELECT id, MIN(name) FROM railway_segments");
    if(q.step() == SQLITE_ROW && q.getRows().column_type(0) != SQLITE_NULL)
    {
        isLine = false;
        graphObjId = q.getRows().get<db_id>(0);
        return true;
    }

    return false;
}
