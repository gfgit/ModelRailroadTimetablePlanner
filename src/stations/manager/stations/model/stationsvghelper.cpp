#include "stationsvghelper.h"

#include <sqlite3pp/sqlite3pp.h>

#include "db_metadata/imagemetadata.h"

#include <QFileDevice>

#include <ssplib/stationplan.h>

#include "stations/station_utils.h"

const char stationTable[] = "stations";
const char stationSVGCol[] = "svg_data";

static ImageMetaData::ImageBlobDevice *loadImage_internal(sqlite3pp::database &db, db_id stationId)
{
    ImageMetaData::ImageBlobDevice *dev = new ImageMetaData::ImageBlobDevice(db.db());
    dev->setBlobInfo(stationTable, stationSVGCol, stationId);
    return dev;
}

bool StationSVGHelper::stationHasSVG(sqlite3pp::database &db, db_id stationId, QString *stNameOut)
{
    sqlite3pp::query q(db);
    q.prepare("SELECT name, (svg_data IS NULL) FROM stations WHERE id=?");
    q.bind(1, stationId);
    if(q.step() != SQLITE_ROW)
        return false;

    if(stNameOut)
        *stNameOut = q.getRows().get<QString>(0);

    //svg_data IS NULL == 0 --> station has SVG image
    return q.getRows().get<int>(1) == 0;
}

bool StationSVGHelper::addImage(sqlite3pp::database &db, db_id stationId, QIODevice *source, QString *errOut)
{
    std::unique_ptr<ImageMetaData::ImageBlobDevice> dest;
    dest.reset(loadImage_internal(db, stationId));

    if(!source || !dest)
    {
        if(errOut)
            *errOut = tr("Null device");
        return false;
    }

    //Make room for storing data and open device
    if(!dest->reserveSizeAndReset(source->size()))
    {
        if(errOut)
            *errOut = dest->errorString();
        return false;
    }

    constexpr int bufSize = 8192;
    char buf[bufSize];
    qint64 size = 0;
    while (!source->atEnd() || size < 0)
    {
        size = source->read(buf, bufSize);
        dest->write(buf, size);
    }

    source->close();
    return true;
}

bool StationSVGHelper::removeImage(sqlite3pp::database &db, db_id stationId, QString *errOut)
{
    sqlite3pp::command cmd(db, "UPDATE stations SET svg_data = NULL WHERE id=?");
    cmd.bind(1, stationId);
    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        if(errOut)
            *errOut = tr("Cannot remove SVG: %1").arg(db.error_msg());
        return false;
    }
    return true;
}

bool StationSVGHelper::saveImage(sqlite3pp::database &db, db_id stationId, QIODevice *dest, QString *errOut)
{
    std::unique_ptr<ImageMetaData::ImageBlobDevice> source;
    source.reset(loadImage_internal(db, stationId));

    if(!source || !dest)
    {
        if(errOut)
            *errOut = tr("Null device");
        return false;
    }

    if(!source->open(QIODevice::ReadOnly))
    {
        if(errOut)
            *errOut = tr("Cannot open source: %1").arg(source->errorString());
        return false;
    }

    //Optimization for files, resize to avoid reallocations
    if(QFileDevice *dev = qobject_cast<QFileDevice *>(dest))
        dev->resize(source->size());

    constexpr int bufSize = 4096;
    char buf[bufSize];
    qint64 size = 0;
    while (!source->atEnd() || size < 0)
    {
        size = source->read(buf, bufSize);
        dest->write(buf, size);
    }

    source->close();
    return true;
}

QIODevice *StationSVGHelper::loadImage(sqlite3pp::database &db, db_id stationId)
{
    return loadImage_internal(db, stationId);
}

bool loadStationLabels(sqlite3pp::database &db, db_id stationId, ssplib::StationPlan *plan)
{
    sqlite3pp::query q(db);

    //Load Gate labels
    q.prepare("SELECT g.id,g.out_track_count,g.name,g.side FROM station_gates g"
              " WHERE g.station_id=?"
              " ORDER BY g.name"
              " LIMIT 100");
    q.bind(1, stationId);

    //const QString  labelFmt = QStringLiteral("%1 (%2)");

    for(auto r : q)
    {
        db_id gateId = r.get<db_id>(0);
        int outTrackCount = r.get<int>(1);
        QChar gateLetter = r.get<const char *>(2)[0];
        utils::Side gateSide = utils::Side(r.get<int>(3));

        Q_UNUSED(outTrackCount)
        Q_UNUSED(gateSide)

        int i = 0;
        for(; i < plan->labels.size(); i++)
        {
            if(plan->labels.at(i).gateLetter == gateLetter)
                break; //Label already exists, fill data
        }
        if(i >= plan->labels.size())
        {
            //Skip gate because it was not registered in SVG
            continue;
        }

        ssplib::LabelItem& item = plan->labels[i];
        item.itemId = gateId;

        sqlite3pp::query sub(db);
        sub.prepare("SELECT s.id, s.name, s.max_speed_kmh, s.type, s.distance_meters,"
                    "g1.station_id, g2.station_id,"
                    "st1.name, st2.name"
                    " FROM railway_segments s"
                    " JOIN station_gates g1 ON g1.id=s.in_gate_id"
                    " JOIN station_gates g2 ON g2.id=s.out_gate_id"
                    " JOIN stations st1 ON st1.id=g1.station_id"
                    " JOIN stations st2 ON st2.id=g2.station_id"
                    " WHERE s.in_gate_id=?1 OR s.out_gate_id=?1");
        sub.bind(1, gateId);
        for(auto s : sub)
        {
            db_id segId = s.get<db_id>(0);
            QString segName = s.get<QString>(1);
            int maxSpeedKmH = s.get<int>(2);
            utils::RailwaySegmentType segType = utils::RailwaySegmentType(s.get<int>(3));
            int distanceMeters = s.get<int>(4);

            db_id fromStationId = s.get<db_id>(5);
            db_id toStationId = s.get<db_id>(6);

            QString fromStationName = s.get<QString>(7);
            QString toStationName = s.get<QString>(8);

            Q_UNUSED(segId)
            Q_UNUSED(maxSpeedKmH)
            Q_UNUSED(segType)
            Q_UNUSED(distanceMeters)

            bool reversed = false;
            if(toStationId == stationId)
            {
                qSwap(fromStationId, toStationId);
                qSwap(fromStationName, toStationName);
                reversed = true;
            }
            else if(fromStationId != stationId)
            {
                //Error: station is neither 'from' nor 'to'
                //Skip segment
                continue;
            }

            Q_UNUSED(reversed)

            //TODO: set segment name as tooltip because otherwise label is too long
            //item.labelText = labelFmt.arg(toStationName, segName);
            Q_UNUSED(segName)
            item.labelText = toStationName;


            //Mark label as visible
            item.visible = true;

            //Found the segment,  break
            break;
        }
    }

    return true;
}

bool loadStationTracks(sqlite3pp::database &db, db_id stationId, ssplib::StationPlan *plan)
{
    sqlite3pp::query q(db);

    //Load Station Tracks
    q.prepare("SELECT id, pos, name FROM station_tracks WHERE station_id=?");
    q.bind(1, stationId);

    for(auto r : q)
    {
        db_id trackId = r.get<db_id>(0);
        int trackPos = r.get<int>(1);
        QString trackName = r.get<QString>(2);

        int i = 0;
        for(; i < plan->platforms.size(); i++)
        {
            if(plan->platforms.at(i).trackPos == trackPos)
                break; //Track already exists, fill data
        }
        if(i >= plan->platforms.size())
        {
            //Skip track because it was not registered in SVG
            continue;
        }

        ssplib::TrackItem& item = plan->platforms[i];
        item.itemId = trackId;
        item.trackName = trackName;
    }

    return true;
}

bool loadStationTrackConnections(sqlite3pp::database &db, db_id stationId, ssplib::StationPlan *plan)
{
    sqlite3pp::query q(db);

    //Load Station Tracks
    q.prepare("SELECT c.id, c.track_id, c.track_side, t.pos,"
              "c.gate_id, g.name, c.gate_track FROM station_gate_connections c"
              " JOIN station_tracks t ON t.id=c.track_id"
              " JOIN station_gates g ON g.id=c.gate_id"
              " WHERE g.station_id=?"
              " LIMIT 100");
    q.bind(1, stationId);

    auto& trackConnections = plan->trackConnections;

    for(auto r : q)
    {
        ssplib::TrackConnectionInfo info;
        db_id connId = r.get<db_id>(0);
        info.trackId = r.get<db_id>(1);
        utils::Side trackSide = utils::Side(r.get<int>(2));
        info.stationTrackPos = r.get<int>(3);
        info.gateId = r.get<db_id>(4);
        info.gateLetter = r.get<const char *>(5)[0];
        info.gateTrackPos = r.get<int>(6);

        Q_UNUSED(trackSide)

        int i = 0;
        for(; i < trackConnections.size(); i++)
        {
            const ssplib::TrackConnectionItem& item = trackConnections.at(i);
            if(item.info == info)
                break; //Track connection already exists, fill data
        }
        if(i >= trackConnections.size())
        {
            //Skip connection because it was not registered in SVG
            continue;
        }

        ssplib::TrackConnectionItem& item = trackConnections[i];
        item.itemId = connId;

        //NOTE: when TrackConnectionInfo matches to another, it checks only some fields
        //So replace it with new data
        item.info = info;
    }

    return true;
}

bool StationSVGHelper::loadStationFromDB(sqlite3pp::database &db, db_id stationId, QString& stName, ssplib::StationPlan *plan)
{
    sqlite3pp::query q(db);
    q.prepare("SELECT name FROM stations WHERE id=?");
    q.bind(1, stationId);
    if(q.step() != SQLITE_ROW)
        return false;
    stName = q.getRows().get<QString>(0);

    if(!loadStationLabels(db, stationId, plan))
        return false;

    if(!loadStationTracks(db, stationId, plan))
        return false;

    if(!loadStationTrackConnections(db, stationId, plan))
        return false;

    return true;
}
