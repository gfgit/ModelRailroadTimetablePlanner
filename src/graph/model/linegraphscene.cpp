#include "linegraphscene.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QDebug>

LineGraphScene::LineGraphScene(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
    mDb(db)
{

}

bool LineGraphScene::loadGraph(db_id objectId, LineGraphType type)
{
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

    if(type == LineGraphType::SingleStation)
    {
        StationGraphObject st;
        if(!loadStation(objectId, st))
            return false;

        //Register a single station at position x = 0.0
        st.xPos = 0;
        stations = {st};
        stationPositions = {{st.stationId, st.xPos}};
    }

    graphObjectId = objectId;
    graphType = type;

    return true;
}

bool LineGraphScene::loadStation(db_id stationId, StationGraphObject& st)
{
    sqlite3pp::query q(mDb);

    q.prepare("SELECT name,short_name,type FROM stations WHERE id=?");
    q.bind(1, stationId);
    if(q.step() != SQLITE_ROW)
    {
        qWarning() << "Graph: invalid station ID" << stationId;
        return false;
    }

    //Load station
    auto row = q.getRows();

    st.stationId = stationId;
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
}
