#include "stationsvghelper.h"

#include <sqlite3pp/sqlite3pp.h>

#include "db_metadata/imagemetadata.h"

#include <QFileDevice>

#include <ssplib/stationplan.h>
#include <ssplib/parsing/stationinfoparser.h>

#include "stations/station_utils.h"

#include "app/session.h"

#include "utils/jobcategorystrings.h"

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

bool loadStationLabels(sqlite3pp::database &db, db_id stationId, ssplib::StationPlan *plan, bool onlyAlreadyPresent)
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
        ssplib::Side gateSide = ssplib::Side(r.get<int>(3));

        int i = 0;
        for(; i < plan->labels.size(); i++)
        {
            if(plan->labels.at(i).gateLetter == gateLetter)
                break; //Label already exists, fill data
        }
        if(i >= plan->labels.size())
        {
            if(onlyAlreadyPresent)
            {
                //Skip gate because it was not registered in SVG
                continue;
            }

            //Create new gate
            ssplib::LabelItem gate;
            gate.gateLetter = gateLetter;
            plan->labels.append(gate);
            i = plan->labels.size() - 1;
        }

        ssplib::LabelItem& item = plan->labels[i];
        item.itemId = gateId;
        item.gateOutTrkCount = outTrackCount;
        item.gateSide = gateSide;

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

bool loadStationTracks(sqlite3pp::database &db, db_id stationId, ssplib::StationPlan *plan, bool onlyAlreadyPresent)
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
            if(onlyAlreadyPresent)
            {
                //Skip track because it was not registered in SVG
                continue;
            }

            //Create new track
            ssplib::TrackItem track;
            track.trackPos = trackPos;
            plan->platforms.append(track);
            i = plan->platforms.size() - 1;
        }

        ssplib::TrackItem& item = plan->platforms[i];
        item.itemId = trackId;
        item.trackName = trackName;
    }

    return true;
}

bool loadStationTrackConnections(sqlite3pp::database &db, db_id stationId, ssplib::StationPlan *plan, bool onlyAlreadyPresent)
{
    sqlite3pp::query q(db);

    //Load Station Tracks
    q.prepare("SELECT c.id, c.track_id, c.track_side, t.pos,"
              "c.gate_id, g.name, c.gate_track FROM station_gate_connections c"
              " JOIN station_tracks t ON t.id=c.track_id"
              " JOIN station_gates g ON g.id=c.gate_id"
              " WHERE g.station_id=?"
              " LIMIT 500");
    q.bind(1, stationId);

    auto& trackConnections = plan->trackConnections;

    for(auto r : q)
    {
        ssplib::TrackConnectionInfo info;
        db_id connId = r.get<db_id>(0);
        info.trackId = r.get<db_id>(1);
        info.trackSide = ssplib::Side(r.get<int>(2));
        info.stationTrackPos = r.get<int>(3);
        info.gateId = r.get<db_id>(4);
        info.gateLetter = r.get<const char *>(5)[0];
        info.gateTrackPos = r.get<int>(6);

        int i = 0;
        for(; i < trackConnections.size(); i++)
        {
            const ssplib::TrackConnectionItem& item = trackConnections.at(i);
            if(item.info.matchNames(info))
                break; //Track connection already exists, fill data
        }
        if(i >= trackConnections.size())
        {
            if(onlyAlreadyPresent)
            {
                //Skip connection because it was not registered in SVG
                continue;
            }

            //Create new gate
            plan->trackConnections.append(ssplib::TrackConnectionItem());
            i = plan->trackConnections.size() - 1;
        }

        ssplib::TrackConnectionItem& item = trackConnections[i];
        item.itemId = connId;

        //NOTE: when TrackConnectionInfo matches to another, it checks only some fields
        //So replace it with new data
        item.info = info;
    }

    return true;
}

bool StationSVGHelper::loadStationFromDB(sqlite3pp::database &db, db_id stationId,
                                         ssplib::StationPlan *plan, bool onlyAlreadyPresent)
{
    sqlite3pp::query q(db);
    q.prepare("SELECT name FROM stations WHERE id=?");
    q.bind(1, stationId);
    if(q.step() != SQLITE_ROW)
        return false;
    plan->stationName = q.getRows().get<QString>(0);

    if(!loadStationLabels(db, stationId, plan, onlyAlreadyPresent))
        return false;

    if(!loadStationTracks(db, stationId, plan, onlyAlreadyPresent))
        return false;

    if(!loadStationTrackConnections(db, stationId, plan, onlyAlreadyPresent))
        return false;

    return true;
}

bool StationSVGHelper::getPrevNextStop(sqlite3pp::database &db, db_id stationId,
                                       bool next, QTime &time)
{
    const QTime origTime = time;

    sqlite3pp::query q(db);
    if(next)
        q.prepare("SELECT MIN(arrival) FROM stops WHERE station_id=? AND arrival>?");
    else
        q.prepare("SELECT MAX(departure) FROM stops WHERE station_id=? AND departure<?");

    q.bind(1, stationId);
    q.bind(2, origTime);
    q.step();
    bool found = q.getRows().column_type(0) != SQLITE_NULL;
    if(found)
        time = q.getRows().get<QTime>(0);

    //Try with swapped arrival and departure
    if(next)
        q.prepare("SELECT MIN(departure) FROM stops WHERE station_id=? AND departure>?");
    else
        q.prepare("SELECT MAX(arrival) FROM stops WHERE station_id=? AND arrival<?");

    q.bind(1, stationId);
    q.bind(2, origTime);
    q.step();
    if(q.getRows().column_type(0) != SQLITE_NULL)
    {
        QTime otherTime = q.getRows().get<QTime>(0);
        //NOTE: If previous query didn't find any stop take this
        //Otherwise compare with previous query result
        //If next, keep earlier one, if previous keep latest one
        if(!found || (next && otherTime < time) || (!next && otherTime > time))
            time = otherTime;
        found = true;
    }

    return found;
}

bool StationSVGHelper::loadStationJobsFromDB(sqlite3pp::database &db, StationSVGJobStops *station)
{
    sqlite3pp::query q(db);
    q.prepare("SELECT stops.id,stops.job_id,jobs.category,stops.arrival,stops.departure,"
              "stops.in_gate_conn,c1.track_id,c1.track_side,c1.gate_id,c1.gate_track,"
              "stops.out_gate_conn,c2.track_id,c2.track_side,c2.gate_id,c2.gate_track,"
              "stops.next_segment_conn_id,stops.type"
              " FROM stops"
              " JOIN jobs ON jobs.id=stops.job_id"
              " LEFT JOIN station_gate_connections c1 ON c1.id=stops.in_gate_conn"
              " LEFT JOIN station_gate_connections c2 ON c2.id=stops.out_gate_conn"
              " WHERE stops.station_id=?1 AND stops.arrival<=?2 AND stops.departure>=?2");
    q.bind(1, station->stationId);
    q.bind(2, station->time);

    station->stops.clear();

    for(auto stop : q)
    {
        StationSVGJobStops::Stop s;
        s.job.stopId = stop.get<db_id>(0);
        s.job.jobId = stop.get<db_id>(1);
        s.job.category = JobCategory(stop.get<int>(2));
        s.arrival = stop.get<QTime>(3);
        s.departure = stop.get<QTime>(4);

        s.in_gate.connId = stop.get<db_id>(5);
        s.in_gate.trackId = stop.get<db_id>(6);
        s.in_gate.trackSide = utils::Side(stop.get<int>(7));
        s.in_gate.gateId = stop.get<db_id>(8);
        s.in_gate.gateTrackNum = stop.get<int>(9);

        s.out_gate.connId = stop.get<db_id>(10);
        s.out_gate.trackId = stop.get<db_id>(11);
        s.out_gate.trackSide = utils::Side(stop.get<int>(12));
        s.out_gate.gateId = stop.get<db_id>(13);
        s.out_gate.gateTrackNum = stop.get<int>(14);

        s.next_seg_conn = stop.get<db_id>(15);
        s.type = StopType(stop.get<db_id>(16));

        station->stops.append(s);
    }

    return true;
}

bool StationSVGHelper::applyStationJobsToPlan(const StationSVGJobStops *station, ssplib::StationPlan *plan)
{
    const QString fmt = tr("<b>%1</b> from <b>%2</b> to <b>%3</b><br>"
                           "Platform: <b>%4</b><br>"
                           "<b>%5</b>");
    const QString statusArr = tr("Arriving");
    const QString statusDep = tr("Departing");
    const QString statusStop = tr("Stop");
    const QString statusTransit = tr("Transit");

    for(const StationSVGJobStops::Stop& stop : station->stops)
    {
        db_id trackId = stop.in_gate.trackId;
        if(!trackId)
            trackId = stop.out_gate.trackId;

        QString tooltip = fmt.arg(JobCategoryName::jobName(stop.job.jobId, stop.job.category),
                                  stop.arrival.toString("HH:mm"), stop.departure.toString("HH:mm"));

        QRgb color = Session->colorForCat(stop.job.category).rgb();

        bool foundPlatform = false;
        for(ssplib::TrackItem &platf : plan->platforms)
        {
            if(platf.itemId != trackId)
                continue;

            foundPlatform = true;
            tooltip = tooltip.arg(platf.trackName);

            QString status = statusStop;
            if(stop.type == StopType::Transit)
                status = statusTransit;
            else if(stop.departure == station->time && stop.next_seg_conn)
                status = statusDep;

            platf.visible = true;
            platf.tooltip = tooltip.arg(status);
            platf.color = color;
            break;
        }

        if(!foundPlatform)
            tooltip = tooltip.arg(tr("Not found", "Station platform was not found in SVG"));

        if(stop.arrival == station->time)
        {
            ssplib::TrackConnectionInfo inConn;
            inConn.gateId       = stop.in_gate.gateId;
            inConn.gateTrackPos = stop.in_gate.gateTrackNum;
            inConn.trackId      = stop.in_gate.trackId;
            inConn.trackSide    = ssplib::Side(stop.in_gate.trackSide);

            //Train is arriving at requested time, show path
            for(ssplib::TrackConnectionItem &conn : plan->trackConnections)
            {
                if(conn.itemId != stop.in_gate.connId && !conn.info.matchIDs(inConn))
                    continue;

                conn.visible = true;
                conn.tooltip = tooltip.arg(statusArr);
                conn.color = color;
                break;
            }
        }

        //Check also next_seg_conn because sometimes gate out connection
        //is used to store track in stop but doesn't mean job will depart.
        if(stop.departure == station->time && stop.next_seg_conn)
        {
            ssplib::TrackConnectionInfo outConn;
            outConn.gateId       = stop.out_gate.gateId;
            outConn.gateTrackPos = stop.out_gate.gateTrackNum;
            outConn.trackId      = stop.out_gate.trackId;
            outConn.trackSide    = ssplib::Side(stop.out_gate.trackSide);

            //Train is departing at requested time, show path
            for(ssplib::TrackConnectionItem &conn : plan->trackConnections)
            {
                if(conn.itemId != stop.out_gate.connId && !conn.info.matchIDs(outConn))
                    continue;

                conn.visible = true;
                conn.tooltip = tooltip.arg(statusDep);
                conn.color = color;
                break;
            }
        }
    }

    return true;
}

bool StationSVGHelper::writeStationXml(QIODevice *dev, ssplib::StationPlan *plan)
{
    ssplib::StationInfoWriter writer(dev);
    return writer.write(plan);
}

bool StationSVGHelper::writeStationXmlFromDB(sqlite3pp::database &db, db_id stationId, QIODevice *dev)
{
    ssplib::StationPlan plan;
    if(!loadStationFromDB(db, stationId, &plan, false))
        return false;
    return writeStationXml(dev, &plan);
}
