#include "linegraphscene.h"

<<<<<<< HEAD
#include "app/session.h"

=======
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e
#include <sqlite3pp/sqlite3pp.h>

#include <QDebug>

LineGraphScene::LineGraphScene(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
<<<<<<< HEAD
    mDb(db),
    graphObjectId(0),
    graphType(LineGraphType::NoGraph)
=======
    mDb(db)
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e
{

}

bool LineGraphScene::loadGraph(db_id objectId, LineGraphType type)
{
<<<<<<< HEAD
    if(!mDb.db())
    {
        qWarning() << "Database not open on graph loading!";
        return false;
    }

=======
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e
    if(objectId <= 0)
    {
        qWarning() << "Invalid object ID on graph loading!";
        return false;
    }

    if(type != LineGraphType::SingleStation)
    {
        qWarning() << "FIXME: only station graph supported now!";
        //FIXME: add support for other types
        return false;
    }

<<<<<<< HEAD
    //Initial state is invalid
    graphType = LineGraphType::NoGraph;
    stations.clear();
    stationPositions.clear();

    if(type == LineGraphType::SingleStation)
    {
        StationGraphObject st;
        st.stationId = objectId;
        if(!loadStation(st))
=======
    if(type == LineGraphType::SingleStation)
    {
        StationGraphObject st;
        if(!loadStation(objectId, st))
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e
            return false;

        //Register a single station at position x = 0.0
        st.xPos = 0;
<<<<<<< HEAD
        stations.insert(st.stationId, st);
        stationPositions = {{st.stationId, st.xPos}};
        graphObjectName = st.stationName;
    }
    else if(type == LineGraphType::RailwaySegment)
    {
        StationGraphObject stA, stB;

        sqlite3pp::query q(mDb,
                           "SELECT s.in_gate_id,s.out_gate_id,s.name,s.max_speed_kmh,"
                           "s.type,s.distance_meters,"
                           "g1.station_id,g2.station_id"
                           " FROM railway_segments s"
                           " JOIN station_gates g1 ON g1.id=s.in_gate_id"
                           " JOIN station_gates g2 ON g2.id=s.out_gate_id"
                           " WHERE s.id=?");
        q.bind(1, objectId);
        if(q.step() != SQLITE_ROW)
        {
            qWarning() << "Graph: invalid segment ID" << objectId;
            return false;
        }

        auto r = q.getRows();
//        TODO useful?
//        outFromGateId = r.get<db_id>(0);
//        outToGateId = r.get<db_id>(1);
        graphObjectName = r.get<QString>(2);
//        outSpeed = r.get<int>(3);
//        outType = utils::RailwaySegmentType(r.get<db_id>(4));
//        outDistance = r.get<int>(5);

        stA.stationId = r.get<db_id>(6);
        stB.stationId = r.get<db_id>(7);

        if(!loadStation(stA) || !loadStation(stB))
            return false;

        stA.xPos = 0.0;
        stB.xPos = stA.platforms.count() * Session->platformOffset + Session->stationOffset;

        stations.insert(stA.stationId, stA);
        stations.insert(stB.stationId, stB);

        stationPositions = {{stA.stationId, stA.xPos},
                            {stB.stationId, stB.xPos}};
=======
        stations = {st};
        stationPositions = {{st.stationId, st.xPos}};
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e
    }

    graphObjectId = objectId;
    graphType = type;

    return true;
}

<<<<<<< HEAD
bool LineGraphScene::loadStation(StationGraphObject& st)
=======
bool LineGraphScene::loadStation(db_id stationId, StationGraphObject& st)
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e
{
    sqlite3pp::query q(mDb);

    q.prepare("SELECT name,short_name,type FROM stations WHERE id=?");
<<<<<<< HEAD
    q.bind(1, st.stationId);
    if(q.step() != SQLITE_ROW)
    {
        qWarning() << "Graph: invalid station ID" << st.stationId;
=======
    q.bind(1, stationId);
    if(q.step() != SQLITE_ROW)
    {
        qWarning() << "Graph: invalid station ID" << stationId;
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e
        return false;
    }

    //Load station
    auto row = q.getRows();

<<<<<<< HEAD
=======
    st.stationId = stationId;
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e
    st.stationName = row.get<QString>(1);
    if(st.stationName.isEmpty())
    {
        //Empty short name, fallback to full name
        st.stationName = row.get<QString>(0);
    }
    st.stationType = utils::StationType(row.get<int>(2));

    //Load platforms
    q.prepare("SELECT id, type, color_rgb, name FROM station_tracks WHERE station_id=? ORDER BY pos");
    for(auto r : q)
    {
        StationGraphObject::PlatformGraph platf;
        platf.platformId = r.get<db_id>(0);
        platf.platformType = utils::StationTrackType(r.get<int>(1));
        platf.color = QRgb(r.get<int>(2)); //TODO: get default color if NULL or white
        platf.platformName = r.get<QString>(3);
        st.platforms.append(platf);
    }
<<<<<<< HEAD

    return true;
=======
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e
}
