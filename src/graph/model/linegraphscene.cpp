#include "linegraphscene.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QDebug>

LineGraphScene::LineGraphScene(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
    mDb(db),
    graphObjectId(0),
    graphType(LineGraphType::NoGraph)
{

}

bool LineGraphScene::loadGraph(db_id objectId, LineGraphType type)
{
    if(!mDb.db())
    {
        qWarning() << "Database not open on graph loading!";
        return false;
    }

    if(objectId <= 0)
    {
        qWarning() << "Invalid object ID on graph loading!";
        return false;
    }

    //Initial state is invalid
    graphType = LineGraphType::NoGraph;
    graphObjectId = 0;
    graphObjectName.clear();
    stations.clear();
    stationPositions.clear();

    if(type == LineGraphType::SingleStation)
    {
        StationGraphObject st;
        st.stationId = objectId;
        if(!loadStation(st))
            return false;

        //Register a single station at position x = 0.0
        st.xPos = 0;
        stations.insert(st.stationId, st);
        stationPositions = {{st.stationId, st.xPos}};
        graphObjectName = st.stationName;
    }
    else if(type == LineGraphType::RailwaySegment)
    {
        //TODO: maybe show also station gates
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
    }
    else if(type == LineGraphType::RailwayLine)
    {
        if(!loadFullLine(objectId))
        {
            stations.clear();
            stationPositions.clear();
            return false;
        }
    }

    graphObjectId = objectId;
    graphType = type;

    emit graphChanged(int(graphType), graphObjectId);
    emit redrawGraph();

    return true;
}

bool LineGraphScene::loadStation(StationGraphObject& st)
{
    sqlite3pp::query q(mDb);

    q.prepare("SELECT name,short_name,type FROM stations WHERE id=?");
    q.bind(1, st.stationId);
    if(q.step() != SQLITE_ROW)
    {
        qWarning() << "Graph: invalid station ID" << st.stationId;
        return false;
    }

    //Load station
    auto row = q.getRows();

    st.stationName = row.get<QString>(1);
    if(st.stationName.isEmpty())
    {
        //Empty short name, fallback to full name
        st.stationName = row.get<QString>(0);
    }
    st.stationType = utils::StationType(row.get<int>(2));

    //Load platforms
    const QRgb white = qRgb(255, 255, 255);
    q.prepare("SELECT id, type, color_rgb, name FROM station_tracks WHERE station_id=? ORDER BY pos");
    q.bind(1, st.stationId);
    for(auto r : q)
    {
        StationGraphObject::PlatformGraph platf;
        platf.platformId = r.get<db_id>(0);
        platf.platformType = utils::StationTrackType(r.get<int>(1));
        if(r.column_type(2) == SQLITE_NULL) //NULL is white (#FFFFFF) -> default value
            platf.color = white;
        else
            platf.color = QRgb(r.get<int>(2));
        platf.platformName = r.get<QString>(3);
        st.platforms.append(platf);
    }

    return true;
}

bool LineGraphScene::loadFullLine(db_id lineId)
{
    //TODO: maybe show also station gates
    //TODO: load only visible stations, other will be loaded when scrolling graph
    sqlite3pp::query q(mDb, "SELECT name FROM lines WHERE id=?");
    q.bind(1, lineId);
    if(q.step() != SQLITE_ROW)
    {
        qWarning() << "Graph: invalid line ID" << lineId;
        return false;
    }

    //Store line name
    graphObjectName = q.getRows().get<QString>(0);

    //Get segments
    q.prepare("SELECT ls.id, ls.seg_id, ls.direction,"
              "seg.name, seg.max_speed_kmh, seg.type, seg.distance_meters,"
              "g1.station_id, g2.station_id"
              " FROM line_segments ls"
              " JOIN railway_segments seg ON seg.id=ls.seg_id"
              " JOIN station_gates g1 ON g1.id=seg.in_gate_id"
              " JOIN station_gates g2 ON g2.id=seg.out_gate_id"
              " WHERE ls.line_id=?"
              " ORDER BY ls.pos");
    q.bind(1, lineId);

    db_id lastStationId = 0;
    double curPos = 0.0;

    for(auto seg : q)
    {
        db_id lineSegmentId = seg.get<db_id>(0);
        //item.railwaySegmentId = seg.get<db_id>(1);
        bool reversed = seg.get<int>(2) != 0;

        //item.segmentName = seg.get<QString>(3);
        //item.maxSpeedKmH = seg.get<int>(4);
        //item.segmentType = utils::RailwaySegmentType(seg.get<int>(5));
        //item.distanceMeters = seg.get<int>(6);

        //Store first segment end
        db_id fromStationId = seg.get<db_id>(7);

        //Store also the other end of segment for last item
        db_id otherStationId = seg.get<db_id>(8);

        if(reversed)
        {
            //Swap segments ends
            qSwap(fromStationId, otherStationId);
        }

        if(!lastStationId)
        {
            StationGraphObject st;
            st.stationId = fromStationId;
            if(!loadStation(st))
                return false;

            st.xPos = 0.0;
            stations.insert(st.stationId, st);
            stationPositions.append({st.stationId, st.xPos});
            curPos = st.platforms.count() * Session->platformOffset + Session->stationOffset;
        }
        else if(fromStationId != lastStationId)
        {
            qWarning() << "Line segments are not adjacent, ID:" << lineSegmentId
                       << "LINE:" << lineId;
            return false;
        }

        StationGraphObject stB;
        stB.stationId = otherStationId;
        if(!loadStation(stB))
            return false;

        stB.xPos = curPos;
        stations.insert(stB.stationId, stB);
        stationPositions.append({stB.stationId, stB.xPos});

        curPos += stB.platforms.count() * Session->platformOffset + Session->stationOffset;
        lastStationId = stB.stationId;
    }

    return true;
}
