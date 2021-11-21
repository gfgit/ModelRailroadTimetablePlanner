#include "stopmodel.h"

#include <QDebug>

#include "app/scopedebug.h"

#include "app/session.h"

#include <QtMath>

StopModel::StopModel(database &db, QObject *parent) :
    QAbstractListModel(parent),
    mJobId(0),
    mNewJobId(0),
    jobShiftId(0),
    newShiftId(0),
    category(JobCategory::FREIGHT),
    oldCategory(JobCategory::FREIGHT),
    editState(NotEditing),
    mDb(db),
    q_segPos(mDb),
    q_getRwNode(mDb),
    q_lineHasSt(mDb),
    q_getCoupled(mDb),
    q_setArrival(mDb),
    q_setDeparture(mDb),
    q_setSegPos(mDb),
    q_setSegLine(mDb),
    q_setStopSeg(mDb),
    q_setNextSeg(mDb),
    q_setStopSt(mDb),
    q_removeSeg(mDb),
    q_setPlatform(mDb),
    timeCalcEnabled(true),
    autoInsertTransits(false),
    autoMoveUncoupleToNewLast(true),
    autoUncoupleAtLast(true)
{
    reloadSettings();
    connect(&AppSettings, &MRTPSettings::stopOptionsChanged, this, &StopModel::reloadSettings);

    connect(Session, &MeetingSession::shiftJobsChanged, this, &StopModel::onExternalShiftChange);
    connect(Session, &MeetingSession::shiftNameChanged, this, &StopModel::onShiftNameChanged);

    connect(Session, &MeetingSession::lineNameChanged, this, &StopModel::onStationLineNameChanged);
    connect(Session, &MeetingSession::stationNameChanged, this, &StopModel::onStationLineNameChanged);
}

void StopModel::prepareQueries()
{


    q_segPos.prepare("SELECT num FROM jobsegments WHERE id=?");

    q_getRwNode.prepare("SELECT railways.id FROM railways"
                        " JOIN jobsegments ON jobsegments.id=?"
                        " WHERE railways.stationId=? AND railways.lineId=jobsegments.lineId");

    q_lineHasSt.prepare("SELECT * FROM railways WHERE lineId=? AND stationId=?");

    q_getCoupled.prepare("SELECT rs_id, operation FROM coupling WHERE stop_id=?");





    q_setArrival.prepare("UPDATE stops SET arrival=? WHERE id=?");
    q_setDeparture.prepare("UPDATE stops SET departure=? WHERE id=?");

    q_setSegPos.prepare("UPDATE jobsegments SET num=num+? WHERE job_id=? AND num>=?");

    q_setSegLine.prepare("UPDATE jobsegments SET lineId=? WHERE id=?");

    q_setStopSeg.prepare("UPDATE stops SET segmentId=?, rw_node=? WHERE id=?");

    q_setNextSeg.prepare("UPDATE stops SET otherSegment=?, other_rw_node=? WHERE id=?");

    q_setStopSt.prepare("UPDATE stops SET stationId=?, rw_node=? WHERE id=?");

    q_removeSeg.prepare("DELETE FROM jobsegments WHERE id=?");

    q_setPlatform.prepare("UPDATE stops SET platform=? WHERE id=?");

}

void StopModel::finalizeQueries()
{
    q_segPos.finish();
    q_getRwNode.finish();
    q_lineHasSt.finish();
    q_getCoupled.finish();

    q_setArrival.finish();
    q_setDeparture.finish();

    q_setSegPos.finish();
    q_setSegLine.finish();
    q_setStopSeg.finish();
    q_setNextSeg.finish();
    q_setStopSt.finish();
    q_removeSeg.finish();
    q_setPlatform.finish();
}

void StopModel::reloadSettings()
{
    setAutoInsertTransits(AppSettings.getAutoInsertTransits());
    setAutoMoveUncoupleToNewLast(AppSettings.getAutoShiftLastStopCouplings());
    setAutoUncoupleAtLast(AppSettings.getAutoUncoupleAtLastStop());
}

const QSet<db_id> &StopModel::getStationsToUpdate() const
{
    return stationsToUpdate;
}

LineType StopModel::getLineTypeAfterStop(db_id stopId) const
{
    int row = getStopRow(stopId);
    if(row == -1)
        return LineType(-1); //Error

    const StopItem& item = stops.at(row);
    query q(mDb, "SELECT type FROM lines WHERE id=?");

    if(item.nextLine)
        q.bind(1, item.nextLine);
    else
        q.bind(1, item.curLine);
    if(q.step() != SQLITE_ROW)
        return LineType(-1); //Error

    LineType type = LineType(q.getRows().get<int>(0));
    return type;
}

void StopModel::uncoupleStillCoupledAtLastStop()
{
    if(!autoUncoupleAtLast)
        return;

    for(int i = stops.size() - 1; i >= 0; i--)
    {
        const StopItem& s = stops.at(i);
        if(s.addHere != 0 || !s.stationId)
            continue;
        uncoupleStillCoupledAtStop(s);

        //Select them to update them
        query q_selectMoved(mDb, "SELECT rs_id FROM coupling WHERE stop_id=?");
        q_selectMoved.bind(1, s.stopId);
        for(auto rs : q_selectMoved)
        {
            db_id rsId = rs.get<db_id>(0);
            rsToUpdate.insert(rsId);
        }
        break;
    }
}

void StopModel::uncoupleStillCoupledAtStop(const StopItem& s)
{
    //Uncouple all still-coupled RS
    command q_uncoupleRS(mDb, "INSERT OR IGNORE INTO coupling(id,rs_id,stop_id,operation) VALUES(NULL,?,?,0)");
    query q_selectStillOn(mDb, "SELECT coupling.rs_id,MAX(stops.arrival)"
                               " FROM stops"
                               " JOIN coupling ON coupling.stop_id=stops.id"
                               " WHERE stops.job_id=?1 AND stops.arrival<?2"
                               " GROUP BY coupling.rs_id"
                               " HAVING coupling.operation=1");
    q_selectStillOn.bind(1, mJobId);
    q_selectStillOn.bind(2, s.arrival);
    for(auto rs : q_selectStillOn)
    {
        db_id rsId = rs.get<db_id>(0);
        rsToUpdate.insert(rsId);
        q_uncoupleRS.bind(1, rsId);
        q_uncoupleRS.bind(2, s.stopId);
        q_uncoupleRS.execute();
        q_uncoupleRS.reset();
    }
}

bool StopModel::getStationPlatfCount(db_id stationId, int &platfCount, int &depotCount)
{
    query q_getPlatfs(mDb, "SELECT platforms,depot_platf FROM stations WHERE id=?");
    q_getPlatfs.bind(1, stationId);
    if(q_getPlatfs.step() != SQLITE_ROW)
    {
        //Error
        return false;
    }
    auto r = q_getPlatfs.getRows();
    platfCount = r.get<int>(0);
    depotCount = r.get<int>(1);
    return true;
}

const QSet<db_id> &StopModel::getRsToUpdate() const
{
    return rsToUpdate;
}

int StopModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : stops.count();
}

Qt::ItemFlags StopModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

QVariant StopModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() >= stops.count() || index.column() > 0)
        return QVariant();

    const StopItem& s = stops.at(index.row());

    switch (role) {
    case JOB_ID_ROLE:
        return mJobId;
    case JOB_SHIFT_ID:
        return jobShiftId;
    case JOB_CATEGORY_ROLE:
        return int(category);
    case STATION_ID:
        return s.stationId;
    case ARR_ROLE:
        return s.arrival;
    case DEP_ROLE:
        return s.departure;
    case NEXT_LINE_ROLE:
        return s.nextLine;
    case ADDHERE_ROLE:
        return s.addHere;
    case PLATF_ID:
        return s.platform;
    case STOP_DESCR_ROLE:
        return getDescription(s);
    default:
        break;
    }

    return QVariant();
}

bool StopModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    //DEBUG_ENTRY;
    if(!index.isValid() || index.row() >= stops.count() || index.column() > 0)
        return false;

    switch (role)
    {
    case STATION_ID:
        setStation(index, value.toLongLong());
        break;
    case STOP_TYPE_ROLE:
        setStopType(index, StopType(value.toInt()));
        break;
    case ARR_ROLE:
        setArrival(index, value.toTime(), false);
        break;
    case DEP_ROLE:
        setDeparture(index, value.toTime(), true);
        break;
    case NEXT_LINE_ROLE:
        setLine(index, value.toLongLong());
        break;
    case PLATF_ID:
        setPlatform(index, value.toInt());
        break;
    case STOP_DESCR_ROLE:
        setDescription(index, value.toString());
        break;
    default:
        return false;
    }

    return true;
}

void StopModel::setArrival(const QModelIndex& idx, const QTime& time, bool setDepTime)
{
    StopItem& s = stops[idx.row()];
    if(s.type == First || s.arrival == time)
        return;

    //Cannot arrive at the same time (or even before) the departure from previous station
    //You shouldn't get here because StopEditor and EditStopDialog already check this
    //but it's correct to have it checked also in the model
    QTime minArrival = stops[idx.row() - 1].departure.addSecs(60);
    if(time < minArrival)
        return;

    startStopsEditing();

    //Mark the station for update
    stationsToUpdate.insert(s.stationId);

    const QTime oldArr = s.arrival;
    s.arrival = time;

    if(s.type == Last && s.departure != time)
    {
        s.departure = time;

        q_setDeparture.bind(1, time);
        q_setDeparture.bind(2, s.stopId);
        q_setDeparture.execute();
        q_setDeparture.reset();
    }
    else
    {
        if(setDepTime || s.arrival >= s.departure || s.type == Transit || s.type == TransitLineChange)
            shiftStopsBy24hoursFrom(oldArr);
    }

    q_setArrival.bind(1, time);
    q_setArrival.bind(2, s.stopId);
    q_setArrival.execute();
    q_setArrival.reset();


    if(s.type == Transit || s.type == TransitLineChange)
    {
        setDeparture(idx, s.arrival, true);
    }
    else if(setDepTime)
    {
        //Shift Departure by the same amount of time to preserve same stop time
        const int diff = oldArr.secsTo(time);
        setDeparture(idx, s.departure.addSecs(diff), true);
    }

    if(s.arrival > s.departure || (s.arrival == s.departure && s.type != Transit && s.type != TransitLineChange))
    {
        int secs = defaultStopTimeSec();

        if(secs == 0 && s.type != Last)
        {
            //If secs is 0, trasform stop in Transit unless it's Last stop
            setStopType(idx, Transit);
        }
        else
        {
            setDeparture(idx, time.addSecs(secs), true);
        }
    }

    emit dataChanged(idx, idx);
}

void StopModel::setDeparture(const QModelIndex& idx, QTime time, bool propagate)
{
    StopItem& s = stops[idx.row()];
    //Don't allow departure < arrival unless it's First Stop:
    //In First stops setting departure sets also arrival
    //So initially departure < arrival but then arrival = departure
    //And the proble disappears
    if(s.type == Last || s.departure == time)
        return;

    if(s.type != First) //Allow to set Departure on First stop (which updates Arrival)
    {
        QTime minDep = s.arrival; //For transits
        if(s.type == Normal)
            minDep = minDep.addSecs(60); //Normal stops, stop for at least 1 minute
        if(time < minDep)
            return; //Invalid Departure time
    }

    startStopsEditing();

    //Mark the station for update
    stationsToUpdate.insert(s.stationId);

    if(s.type == Transit || s.type == TransitLineChange)
        time = s.arrival; //On transits stop time is 0 minutes so Departure is same of Arrival

    if(s.type == First && s.arrival != time)
    {
        s.arrival = time;

        q_setArrival.bind(1, time);
        q_setArrival.bind(2, s.stopId);
        q_setArrival.execute();
        q_setArrival.reset();
    }

    if(idx.row() < stops.count() - 1) //Not last stop
    {
        StopItem& next = stops[idx.row() + 1];
        if(next.addHere == 0 && time >= s.arrival)
            propagate = true;
    }

    if(propagate)
        shiftStopsBy24hoursFrom(s.arrival);

    q_setDeparture.bind(1, time);
    q_setDeparture.bind(2, s.stopId);
    q_setDeparture.execute();
    q_setDeparture.reset();

    int offset = s.departure.msecsTo(time);
    s.departure = time;

    QModelIndex finalIdx;
    if(propagate)
    {
        int r = propageteTimeOffset(idx.row() + 1, offset);
        finalIdx = index(r - 1, 0);
    }
    if(!finalIdx.isValid())
        finalIdx = idx;

    emit dataChanged(idx, finalIdx);
}

int StopModel::propageteTimeOffset(int row, const int msecOffset)
{
    if(row >= stops.count() || msecOffset == 0)
        return row;

    if(row > 0) //Not first
        shiftStopsBy24hoursFrom(stops.at(row - 1).arrival);

    while (row < stops.count())
    {
        StopItem& s = stops[row];
        if(s.addHere != 0)
        {
            row++;
            continue;
        }

        s.arrival = s.arrival.addMSecs(msecOffset);

        q_setArrival.bind(1, s.arrival);
        q_setArrival.bind(2, s.stopId);
        q_setArrival.execute();
        q_setArrival.reset();

        s.departure = s.departure.addMSecs(msecOffset);

        q_setDeparture.bind(1, s.departure);
        q_setDeparture.bind(2, s.stopId);
        q_setDeparture.execute();
        q_setDeparture.reset();

        row++;
    }

    return row;
}

//Returns error codes of type ErrorCodes
int StopModel::setStopType(const QModelIndex& idx, StopType type)
{
    if(!idx.isValid() || idx.row() >= stops.count())
        return ErrorInvalidIndex;
    StopItem& s = stops[idx.row()];

    if(s.type == First || s.type == Last)
    {
        qWarning() << "Error: tried change type of First/Last stop:" << idx << s.stopId << "Job:" << mJobId;
        return ErrorFirstLastTransit;
    }

    //Fix possible wrong transit types
    if(s.nextLine && type == Transit)
        type = TransitLineChange;
    else if (s.nextLine == 0 && type == TransitLineChange)
        type = Transit;

    if(s.type == type)
        return NoError;

    //Cannot couple or uncouple in transits
    if(type == Transit || type == TransitLineChange)
    {
        q_getCoupled.bind(1, s.stopId);
        int res = q_getCoupled.step();
        q_getCoupled.reset();

        if(res == SQLITE_ROW)
        {
            qWarning() << "Error: trying to set Transit on stop:" << s.stopId << "Job:" << mJobId
                       << "while having coupling operation for this stop";
            return ErrorTransitWithCouplings;
        }
        if(res != SQLITE_OK && res != SQLITE_DONE)
        {
            qWarning() << "Error while setting stopType for stop:" << s.stopId << "Job:" << mJobId
                       << "DB Err:" << res << mDb.error_msg() << mDb.extended_error_code();
        }
    }

    startStopsEditing();

    //Mark the station for update
    stationsToUpdate.insert(s.stationId);


    command q_setTransitType(mDb, "UPDATE stops SET transit=? WHERE id=?");

    if(type == Transit || type == TransitLineChange)
        q_setTransitType.bind(1, Transit); //1 = Transit
    else
        q_setTransitType.bind(1, Normal); //0 = Normal
    q_setTransitType.bind(2, s.stopId);
    q_setTransitType.execute();
    q_setTransitType.finish();

    s.type = type;

    if(s.type == Transit || s.type == TransitLineChange)
    {
        //Transit don't stop so departure is the same of arrival -> stop time = 0 minutes
        setDeparture(idx, s.arrival, true);
    }
    else
    {
        if(s.arrival == s.departure)
        {
            int stopSecs = qMax(60, defaultStopTimeSec()); //Default stop time but at least 1 minute
            setDeparture(idx, s.arrival.addSecs(stopSecs), true);
        }
    }

    //TODO: should be already emitted by setDeparture()
    emit dataChanged(idx, idx);

    return NoError;
}

//Returns error codes of type ErrorCodes
int StopModel::setStopTypeRange(int firstRow, int lastRow, StopType type)
{
    if(firstRow < 0 || firstRow > lastRow || lastRow >= stops.count())
        return ErrorInvalidIndex;

    if(type == First || type == Last)
        return ErrorInvalidArgument;

    if(type == TransitLineChange)
        type = Transit;

    int defaultStopMsec = qMax(60, defaultStopTimeSec()) * 1000; //At least 1 minute

    StopType destType = type;

    shiftStopsBy24hoursFrom(stops.at(firstRow).arrival);

    command q_setArrDep(mDb, "UPDATE stops SET arrival=?,departure=? WHERE id=?");
    command q_setTransitType(mDb, "UPDATE stops SET transit=? WHERE id=?");

    int msecOffset = 0;

    for(int r = firstRow; r <= lastRow; r++)
    {
        StopItem& s = stops[r];

        if(s.type == First || s.type == Last)
        {
            qWarning() << "Error: tried change type of First/Last stop:" << r << s.stopId << "Job:" << mJobId;

            //Always update time even if msecOffset == 0, because they have been shifted
            s.arrival = s.arrival.addMSecs(msecOffset);
            s.departure = s.departure.addMSecs(msecOffset);
            q_setArrDep.bind(1, s.arrival);
            q_setArrDep.bind(2, s.departure);
            q_setArrDep.bind(3, s.stopId);
            q_setArrDep.execute();
            q_setArrDep.reset();
            continue;
        }

        if(type == ToggleType)
        {
            if(s.type == Normal)
                destType = Transit;
            else
                destType = Normal;
        }

        //Cannot couple or uncouple in transits
        if(destType == Transit)
        {
            q_getCoupled.bind(1, s.stopId);
            int res = q_getCoupled.step();
            q_getCoupled.reset();

            if(res == SQLITE_ROW)
            {
                qWarning() << "Error: trying to set Transit on stop:" << s.stopId << "Job:" << mJobId
                           << "while having coupling operation for this stop";
                continue;
            }
            if(res != SQLITE_OK && res != SQLITE_DONE)
            {
                qWarning() << "Error while setting stopType for stop:" << s.stopId << "Job:" << mJobId
                           << "DB Err:" << res << mDb.error_msg() << mDb.extended_error_code();
                return ErrorInvalidArgument;
            }
        }

        startStopsEditing();

        //Mark the station for update
        stationsToUpdate.insert(s.stationId);

        if(destType == Transit)
            q_setTransitType.bind(1, Transit); //1 = Transit
        else
            q_setTransitType.bind(1, Normal); //0 = Normal
        q_setTransitType.bind(2, s.stopId);
        q_setTransitType.execute();
        q_setTransitType.reset();

        s.arrival = s.arrival.addMSecs(msecOffset);
        s.departure = s.departure.addMSecs(msecOffset);

        if(destType == Normal)
        {
            s.type = Normal;

            if(s.arrival == s.departure)
            {
                msecOffset += defaultStopMsec;
                s.departure = s.arrival.addMSecs(defaultStopMsec);
            }
        }
        else
        {
            s.type = s.nextLine ? TransitLineChange : Transit;
            //Transit don't stop so departure is the same of arrival -> stop time = 0 minutes
            msecOffset -= s.arrival.msecsTo(s.departure);
            s.departure = s.arrival;
        }
        q_setArrDep.bind(1, s.arrival);
        q_setArrDep.bind(2, s.departure);
        q_setArrDep.bind(3, s.stopId);
        q_setArrDep.execute();
        q_setArrDep.reset();
    }

    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(stops.count() - 1, 0);

    //Always update time even if msecOffset == 0, because they have been shifted
    for(int r = lastRow + 1; r < stops.count(); r++)
    {
        StopItem& s = stops[r];
        s.arrival = s.arrival.addMSecs(msecOffset);
        s.departure = s.departure.addMSecs(msecOffset);
        q_setArrDep.bind(1, s.arrival);
        q_setArrDep.bind(2, s.departure);
        q_setArrDep.bind(3, s.stopId);
        q_setArrDep.execute();
        q_setArrDep.reset();
    }



    //TODO: should be already emitted by setDeparture()
    emit dataChanged(firstIdx, lastIdx);

    return NoError;
}

void StopModel::loadJobStops(db_id jobId)
{
    DEBUG_ENTRY;

    beginResetModel();
    stops.clear();
    rsToUpdate.clear();
    stationsToUpdate.clear();

    mNewJobId = mJobId = jobId;
    emit jobIdChanged(mJobId);

    {
        query q_getCatAndShift(mDb, "SELECT category,shift_id FROM jobs WHERE id=?");
        q_getCatAndShift.bind(1, mJobId);
        if(q_getCatAndShift.step() != SQLITE_ROW)
        {
            //Error: job not existent???
        }

        auto r = q_getCatAndShift.getRows();
        oldCategory = category = JobCategory(r.get<int>(0));
        emit categoryChanged(int(category));

        jobShiftId = newShiftId = r.get<db_id>(1);
        emit jobShiftChanged(jobShiftId);
    }


    query q_selectStops(mDb, "SELECT COUNT(id) FROM stops WHERE job_id=?");
    q_selectStops.bind(1, mJobId);
    if(q_selectStops.step() != SQLITE_ROW)
    {
        qWarning() << database_error(mDb).what();
    }
    int count = q_selectStops.getRows().get<int>(0);
    q_selectStops.finish();

    if(count == 0)
    {
        endResetModel();

        insertAddHere(0, 1);
        stops.squeeze();

        startStopsEditing();

        return;
    }

    stops.reserve(count);

    int i = 0;

    StopItem::Segment prevSegment;
    db_id prevOutGateId = 0;

    q_selectStops.prepare("SELECT stops.id, stops.station_id, stops.arrival, stops.departure, stops.type,"
                          "stops.in_gate_conn, g1.gate_id, g1.gate_track, g1.track_id,"
                          "stops.out_gate_conn, g2.gate_id, g2.gate_track, g2.track_id,"
                          "stops.next_segment_conn_id, c.seg_id, c.in_track, c.out_track,"
                          "seg.in_gate_id, seg.out_gate_id"
                          " FROM stops"
                          " LEFT JOIN railway_connections c ON c.id=stops.next_segment_conn_id"
                          " LEFT JOIN station_gate_connections g1 ON g1.id=stops.in_gate_conn"
                          " LEFT JOIN station_gate_connections g2 ON g2.id=stops.out_gate_conn"
                          " LEFT JOIN railway_segments seg ON seg.id=c.seg_id"
                          " WHERE stops.job_id=?1"
                          " ORDER BY stops.arrival ASC");
    q_selectStops.bind(1, mJobId);
    for(auto stop : q_selectStops)
    {
        StopItem s;
        s.stopId = stop.get<db_id>(0);
        s.stationId = stop.get<db_id>(1);

        s.arrival = stop.get<QTime>(2);
        s.departure = stop.get<QTime>(3);

        int stopType = stop.get<int>(4);

        s.fromGate.gateConnId = stop.get<db_id>(5);
        s.fromGate.gateId = stop.get<db_id>(6);
        s.fromGate.trackNum = stop.get<int>(7);
        s.trackId = stop.get<db_id>(8);

        s.toGate.gateConnId = stop.get<db_id>(9);
        s.toGate.gateId = stop.get<db_id>(10);
        s.toGate.trackNum = stop.get<int>(11);
        db_id otherTrackId = stop.get<db_id>(12);

        s.nextSegment.segConnId = stop.get<db_id>(13);
        s.nextSegment.segmentId = stop.get<db_id>(14);
        int nextSegInTrack = stop.get<db_id>(15);
        s.nextSegment.outTrackNum = stop.get<db_id>(16);

        db_id segInGateId = stop.get<db_id>(17);
        db_id segOutGateId = stop.get<db_id>(18);

        if(s.toGate.gateId && s.toGate.gateId == segOutGateId)
        {
            //Segment is reversed
            qSwap(segInGateId, segOutGateId);
            qSwap(nextSegInTrack, s.nextSegment.outTrackNum);
            s.nextSegment.reversed = true;
        }

        //Fix station track on First stop
        if(!s.fromGate.gateConnId)
        {
            //If station has no 'in' connection use 'out' connection
            //This might happen on first stop
            s.trackId = otherTrackId;
        }

        //Check consistency
        if(s.trackId != otherTrackId && s.toGate.gateConnId)
        {
            //Last stop has no 'out' connection so do not check track if on 'Last' stop
            //In gate leads to a different station track than out gate
            qWarning() << "Stop:" << s.stopId << "Different track:" << s.fromGate.gateConnId << s.toGate.gateConnId;
        }
        if(prevSegment.segmentId != 0)
        {
            if(prevOutGateId != s.fromGate.gateId)
            {
                //Previous segment leads to a different in gate
                qWarning() << "Stop:" << s.stopId << "Different prev segment:" << prevSegment.segConnId << s.fromGate.gateConnId;
            }

            if(segInGateId != s.toGate.gateId)
            {
                //Out gate leads to a different next semgent
                qWarning() << "Stop:" << s.stopId << "Different next segment:" << s.nextSegment.segConnId << s.toGate.gateConnId;
            }

            if(s.fromGate.trackNum != prevSegment.outTrackNum)
            {
                //Previous segment leads to a different track than in gate track
                qWarning() << "Stop:" << s.stopId << "Different in gate track:" << s.fromGate.gateConnId << s.toGate.gateConnId;
            }

            if(s.toGate.trackNum != nextSegInTrack)
            {
                //Out gate leads to a different than next segment in track
                qWarning() << "Stop:" << s.stopId << "Different out gate track:" << s.toGate.gateConnId << s.nextSegment.segConnId;
            }
        }

        prevSegment = s.nextSegment;
        prevOutGateId = segOutGateId;

        StopType type = Normal;

        if(i == 0)
        {
            type = First;
            if(stopType != 0)
            {
                //Error First cannot be a transit
                qWarning() << "Error: First stop cannot be transit! Job:" << mJobId << "StopId:" << s.stopId;
            }
        }
        else if (stopType)
        {
            type = Transit;

            if(i == count - 1)
            {
                //Error Last cannot be a transit
                qWarning() << "Error: Last stop cannot be transit! Job:" << mJobId << "StopId:" << s.stopId;
            }
        }
        s.type = type;

        stops.append(s);
        i++;
    }
    q_selectStops.finish();

    stops.last().type = Last;

    endResetModel();

    insertAddHere(stops.count(), 1);

    if(stops.count() < 3) //First + Last + AddHere
        startStopsEditing(); //Set editied to enable deletion if user clicks Cancel

    stops.squeeze();
    rsToUpdate.squeeze();
    stationsToUpdate.squeeze();
}

void StopModel::clearJob()
{
    mJobId = mNewJobId = 0;
    emit jobIdChanged(mJobId);

    jobShiftId = newShiftId = 0;
    emit jobShiftChanged(jobShiftId);

    oldCategory = category = JobCategory(-1);
    emit categoryChanged(int(category));

    int count = stops.count();
    if(count == 0)
        return;
    beginRemoveRows(QModelIndex(), 0, count - 1);

    stops.clear();
    stops.squeeze();

    endRemoveRows();
}

void StopModel::insertAddHere(int row, int type)
{
    DEBUG_ENTRY;

    beginInsertRows(QModelIndex(), row, row);

    StopItem s;
    s.addHere = type;
    stops.insert(row, s);

    endInsertRows();
}

db_id StopModel::createStop(db_id jobId, const QTime& arr, const QTime& dep, int type)
{
    command q_addStop(mDb, "INSERT INTO stops"
                           "(id,job_id,stationId,arrival,departure,type,description,"
                           " in_gate_conn,out_gate_conn,next_segment_conn_id)"
                           " VALUES (NULL,?,NULL,?,?,?,NULL,NULL,NULL,NULL)");
    q_addStop.bind(1, jobId);
    q_addStop.bind(2, arr);
    q_addStop.bind(3, dep);
    q_addStop.bind(4, type);

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    q_addStop.execute();
    db_id stopId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_addStop.reset();

    return stopId;
}

db_id StopModel::createSegment(db_id jobId, int num)
{
    command q_addSegment(mDb, "INSERT INTO jobsegments(id,job_id,lineId,num) VALUES (NULL,?,NULL,?)");
    q_addSegment.bind(1, jobId);
    q_addSegment.bind(2, num);

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    q_addSegment.execute();
    db_id segId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);

    q_addSegment.reset();

    return segId;
}

db_id StopModel::createSegmentAfter(db_id jobId, db_id prevSeg)
{
    q_segPos.bind(1, prevSeg);
    q_segPos.step();

    /*Get seg pos and then increment by 1 to get nextSeg pos*/
    int pos = q_segPos.getRows().get<int>(0) + 1;
    q_segPos.reset();

    q_setSegPos.bind(1, 1); //Shift by +1
    q_setSegPos.bind(2, jobId);
    q_setSegPos.bind(3, pos);
    q_setSegPos.execute();
    q_setSegPos.reset();

    return createSegment(jobId, pos);
}

void StopModel::destroySegment(db_id segId, db_id jobId)
{
    q_segPos.bind(1, segId); //Get segment pos, increment to get next segments
    if(q_segPos.step() != SQLITE_ROW)
    {
        qWarning() << "JobId" << jobId << "Segment:" << segId << "Err: tryed to destroy segment that doesn't exist";
        q_segPos.reset();
        return;
    }
    int pos = q_segPos.getRows().get<int>(0) + 1;
    q_segPos.reset();

    q_removeSeg.bind(1, segId); //Remove segment
    int ret = q_removeSeg.execute();
    if(ret != SQLITE_OK)
    {
        qWarning() << "DB err:" << ret << mDb.error_code() << mDb.error_msg() << mDb.extended_error_code();
    }
    q_removeSeg.reset();

    q_setSegPos.bind(1, -1); //Shift by -1 to fill hole in 'num' column
    q_setSegPos.bind(2, jobId);
    q_setSegPos.bind(3, pos);
    q_setSegPos.execute();
    q_setSegPos.reset();
}

void StopModel::deleteStop(db_id stopId)
{
    command q_removeStop(mDb, "DELETE FROM stops WHERE id=?");
    q_removeStop.bind(1, stopId);
    int ret = q_removeStop.execute();
    if(ret != SQLITE_OK)
    {
        qWarning() << "DB err:" << ret << mDb.error_code() << mDb.error_msg() << mDb.extended_error_code();
    }
    q_removeStop.reset();
}

void StopModel::addStop()
{
    DEBUG_IMPORTANT_ENTRY;
    if(stops.count() == 0)
        return;

    startStopsEditing();

    int idx = stops.count() - 1;
    StopItem& last = stops[idx];

    if(last.addHere != 1)
    {
        qWarning() << "Error: addStop not an AddHere Item";
    }

    last.addHere = 0; //Reset AddHere

    int prevIdx = idx - 1;
    if(prevIdx >= 0) //Has stops before
    {
        last.type = Last;

        StopItem& s = stops[prevIdx];
        if(prevIdx > 0)
        {
            s.type = Normal;
            //Previously 's' was Last so it CANNOT have a NextLine
            last.curLine = s.curLine;

            int secs = defaultStopTimeSec();

            if(secs == 0 && s.type != Last)
            {
                //If secs is 0, trasform stop in Transit unless it's Last stop
                setStopType(index(prevIdx, 0), Transit);
            }
            else
            {
                //Don't propagate because we have to initialize last stop before
                setDeparture(index(prevIdx, 0), s.arrival.addSecs(secs), false);
            }
        }
        else
        {
            //First has olny NextLine
            last.curLine = s.nextLine;
        }

        /* Next stop must be at least one minute after
         * This is to prevent contemporary stops that will break ORDER BY arrival queries */
        const QTime time = s.departure.addSecs(60);
        last.arrival = time;
        last.departure = last.arrival;
        last.stopId = createStop(mJobId, last.arrival, last.departure, 0);

        if(autoMoveUncoupleToNewLast)
        {
            if(prevIdx >= 1)
            {
                //We are new last stop and previous is not First (>= 1)
                //Move uncoupled rs from former last stop (now last but one) to new last stop
                command q_moveUncoupled(mDb, "UPDATE OR IGNORE coupling SET stop_id=? WHERE stop_id=? AND operation=?");
                q_moveUncoupled.bind(1, last.stopId);
                q_moveUncoupled.bind(2, s.stopId);
                q_moveUncoupled.bind(3, RsOp::Uncoupled);
                int ret = q_moveUncoupled.execute();
                if(ret != SQLITE_OK)
                {
                    qDebug() << "Error shifting uncoupling from stop:" << s.stopId << "to:" << last.stopId << "Job:" << mJobId
                             << "err:" << ret << mDb.error_msg();

                }
            }

            uncoupleStillCoupledAtStop(last);

            //Select them to update them
            query q_selectMoved(mDb, "SELECT rs_id FROM coupling WHERE stop_id=?");
            q_selectMoved.bind(1, last.stopId);
            for(auto rs : q_selectMoved)
            {
                db_id rsId = rs.get<db_id>(0);
                rsToUpdate.insert(rsId);
            }
        }

        emit dataChanged(index(prevIdx, 0),
                         index(prevIdx, 0));
    }
    else
    {
        last.type = First;

        last.arrival = QTime(0, 0);
        last.departure = last.arrival;
        last.stopId = createStop(mJobId, last.arrival, last.departure, 0);
    }

    emit dataChanged(index(idx, 0),
                     index(idx, 0));

    insertAddHere(idx + 1, 1); //Insert only at end because may invalidate StopItem& references due to mem realloc
}

bool StopModel::lineHasSt(db_id lineId, db_id stId)
{
    q_lineHasSt.bind(1, lineId);
    q_lineHasSt.bind(2, stId);
    int res = q_lineHasSt.step();
    q_lineHasSt.reset();

    return (res == SQLITE_ROW);
}

void StopModel::setStopSeg(StopItem& s, db_id segId)
{
    s.segment = segId;

    q_getRwNode.bind(1, segId);
    q_getRwNode.bind(2, s.stationId);
    q_getRwNode.step();
    db_id nodeId = q_getRwNode.getRows().get<db_id>(0);
    q_getRwNode.reset();

    q_setStopSeg.bind(1, segId);
    if(nodeId)
        q_setStopSeg.bind(2, nodeId);
    else
        q_setStopSeg.bind(2, null_type{}); //Bind NULL instead of 0
    q_setStopSeg.bind(3, s.stopId);
    q_setStopSeg.execute();
    q_setStopSeg.reset();
}

void StopModel::setNextSeg(StopItem& s, db_id nextSeg)
{
    s.nextSegment_ = nextSeg;
    if(nextSeg == 0)
    {
        q_setNextSeg.bind(1, null_type{}); //Bind NULL
        q_setNextSeg.bind(2, null_type{}); //Bind NULL
    }
    else
    {
        q_getRwNode.bind(1, nextSeg);
        q_getRwNode.bind(2, s.stationId);
        q_getRwNode.step();
        db_id nodeId = q_getRwNode.getRows().get<db_id>(0);
        q_getRwNode.reset();

        q_setNextSeg.bind(1, nextSeg);
        if(nodeId)
            q_setNextSeg.bind(2, nodeId);
        else
            q_setNextSeg.bind(2, null_type{}); //Bind NULL instead of 0
    }

    q_setNextSeg.bind(3, s.stopId);
    q_setNextSeg.execute();
    q_setNextSeg.reset();
}

void StopModel::resetStopsLine(int idx, StopItem& s)
{
    //Cannot reset nextLine on First stop
    if(s.type == First)
        return;

    startStopsEditing();

    if(s.type == TransitLineChange)
        s.type = Transit;

    int r = idx + 1;
    for(; r < stops.count(); r++)
    {
        StopItem& stop = stops[r];

        if(s.addHere == 2)
        {
            break;
        }
        if((stop.curLine != s.curLine && stop.segment != s.nextSegment_) || stop.addHere == 1)
        {
            break; //It's an AddHere or in another line so we break loop.
        }

        if(!lineHasSt(s.curLine, stop.stationId) && stop.stationId != 0)
        {
            break;
        }

        stop.curLine = s.curLine;

        db_id oldSegment = stop.segment;
        setStopSeg(stop, s.segment);

        //Check if next stop is using this segment otherwise delete it.
        //If we are last stop just delete it.
        //But if it's equal to s.nextSegment do not delete it because stop 's' still holds a reference to it,
        //it gets deleted anyway after the loop.
        if(oldSegment != s.segment && oldSegment != s.nextSegment_ && (r == stops.count() - 2 || stops[r + 1].segment != oldSegment))
        {
            destroySegment(oldSegment, mJobId);
        }

        if(stop.nextLine == s.curLine)
        {
            //Reset next segment
            db_id oldNextSeg = stop.nextSegment_;
            setNextSeg(stop, 0);
            stop.nextLine = 0;

            //Check if next stop is using this segment otherwise delete it.
            if(r == stops.count() - 2 || stops[r + 1].segment != oldNextSeg)
            {
                destroySegment(oldNextSeg, mJobId);
            }
        }

        if((stop.nextLine != s.curLine && stop.nextLine != 0) || stop.stationId == 0)
        {
            break;
        }
    }

    //First reset next segment on previous stop
    db_id oldNextSeg = s.nextSegment_;
    setNextSeg(s, 0);
    s.nextLine = 0;

    //Only now that there aren't any references to s.nextSegment we can destroy it.
    destroySegment(oldNextSeg, mJobId);

    emit dataChanged(index(idx,   0),
                     index(r, 0));
}

void StopModel::propagateLineChange(int idx, StopItem& s, db_id lineId)
{
    startStopsEditing();

    if(s.type == Transit)
        s.type = TransitLineChange;

    s.nextLine = lineId;
    if(s.type == First)
    {
        q_setSegLine.bind(1, lineId);
        q_setSegLine.bind(2, s.segment);
        q_setSegLine.execute();
        q_setSegLine.reset();

        //Update rw_node field
        setStopSeg(s, s.segment);
    }
    else
    {
        db_id nextSeg = s.nextSegment_;
        if(s.nextSegment_ == 0)
        {
            nextSeg = createSegmentAfter(mJobId, s.segment);
        }

        q_setSegLine.bind(1, lineId);
        q_setSegLine.bind(2, nextSeg);
        q_setSegLine.execute();
        q_setSegLine.reset();

        //Update other_rw_node field
        setNextSeg(s, nextSeg);
    }

    bool endHere = false;

    int r = idx + 1;
    for(; r < stops.count(); r++)
    {
        StopItem& stop = stops[r];

        if(stop.addHere != 0)
        {
            endHere = true;
            break;
        }

        if(stop.stationId != 0 && !lineHasSt(lineId, stop.stationId))
        {
            //Create a new segment for this stop and next on this old segment
            //Because they are in another line

            db_id oldSeg = stop.segment;
            db_id newSeg = createSegmentAfter(mJobId, oldSeg);

            q_setSegLine.bind(1, stop.curLine);
            q_setSegLine.bind(2, newSeg);
            q_setSegLine.execute();
            q_setSegLine.reset();

            //Point prev stop.next... to this stop
            StopItem& prev = stops[r - 1];
            prev.nextLine = stop.curLine;
            setNextSeg(prev, newSeg);

            setStopSeg(stop, newSeg);

            r += 1;
            for(; r < stops.count(); r++)
            {
                StopItem& next = stops[r];
                if(next.addHere != 0)
                {
                    break;
                }

                setStopSeg(next, newSeg);
                if(next.nextSegment_ != 0)
                {
                    break;
                }
            }

            endHere = true;
            break;
        }

        stop.curLine = lineId;

        db_id oldCurSeg = stop.segment;
        setStopSeg(stop, s.type == First ? s.segment : s.nextSegment_);

        /*
         * 'stop' is on same line as 's' so we reset 'stop' nextSegment because it's the same of current segment
         * then obviously we reset nextLine
         */
        if(stop.nextLine == lineId)
        {
            stop.nextLine = 0;
            setNextSeg(stop, 0);
        }
        else
        {
            bool destroy = oldCurSeg != stop.segment && stop.nextSegment_ != oldCurSeg;
            if(destroy && r > 0)
            {
                const StopItem& prev = stops[r - 1];
                if(s.segment == oldCurSeg ||
                    s.nextSegment_ == oldCurSeg ||
                    prev.segment == oldCurSeg)
                {
                    destroy = false;
                }
            }
            if(destroy && stop.type != Last)
            {
                //If stop isn't Last check if oldCurSeg is used by nextStops
                if(stops[r + 1].segment == oldCurSeg)
                    destroy = false;
            }

            if(destroy)
            {
                //oldCurSeg is not used anymore by this job
                destroySegment(oldCurSeg, mJobId);
            }
        }

        if(stop.type == Last && stop.nextSegment_ != 0)
        {
            //Clean up if needed (Should not be needed but just in case)

            //If they are different nextSegment isn't used otherwise don't destroy it
            //because it is used by previous stop
            if(stop.nextSegment_ != stop.segment)
                destroySegment(stop.nextSegment_, mJobId);
            stop.nextLine = 0;
            stop.nextSegment_ = 0;
        }

        if(stop.type == Last || stop.nextLine != 0 || stop.stationId == 0)
        {
            endHere = true;
            break;
        }
    }

    if(endHere)
        return;

    for(int i = r; i < stops.count(); i++)
    {
        StopItem& stop = stops[i];
        if(stop.addHere != 0)
            break;

        if(stop.nextLine != 0)
            break;
    }

    emit dataChanged(index(idx,   0),
                     index(r, 0));
}

void StopModel::setLine(const QModelIndex& idx, db_id lineId)
{
    DEBUG_IMPORTANT_ENTRY;
    if(!idx.isValid() || idx.row() >= stops.count())
        return;
    qDebug() << "Setting line: stop" << idx.row() << "To Id:" << lineId;

    StopItem& s = stops[idx.row()];
    if(s.type == Last || s.addHere != 0 || s.nextLine == lineId)
        return;

    if(s.nextLine == 0 && (s.curLine == lineId || lineId == 0))
        return;

    if(s.nextLine != 0 && (s.curLine == lineId || lineId == 0))
    {
        resetStopsLine(idx.row(), s); //Reset
    }
    else
    {
        propagateLineChange(idx.row(), s, lineId);
    }
}

#ifdef ENABLE_AUTO_TIME_RECALC
void StopModel::rebaseTimesToSpeed(int firstIdx, QTime firstArr, QTime firstDep)
{
    //Recalc times from this stop until last, also apply offset with new departure
    if(firstIdx < 0 || firstIdx >= stops.size() - 2) //At least one before Last stop
        return; //Error

    startStopsEditing();

    StopItem& firstStop = stops[firstIdx];

    QTime minArrival;
    if(firstIdx > 0)
        minArrival = stops.at(firstIdx - 1).departure.addSecs(60);

    if(firstArr < minArrival)
    {
        firstDep = firstDep.addSecs(minArrival.secsTo(firstArr));
    }

    if(firstDep < firstArr)
    {
        firstDep = firstArr.addSecs(60); //At least 1 minute stop, it cannot be a transit if we couple RS
    }

    int offsetSecs = firstStop.departure.secsTo(firstDep);
    firstStop.arrival = firstArr;
    firstStop.departure = firstDep;

    query q(Session->m_Db, "SELECT MIN(rs_models.max_speed), rs_id FROM("
                           "SELECT coupling.rs_id AS rs_id, MAX(stops.arrival)"
                           " FROM stops"
                           " JOIN coupling ON coupling.stop_id=stops.id"
                           " WHERE stops.job_id=? AND stops.arrival<?"
                           " GROUP BY coupling.rs_id"
                           " HAVING coupling.operation=1)"
                           " JOIN rs_list ON rs_list.id=rs_id"
                           " JOIN rs_models ON rs_models.id=rs_list.model_id");

    QTime prevDep = firstDep;
    db_id prevStId = firstStop.stationId;

    db_id lineId = firstStop.nextLine ? firstStop.nextLine : firstStop.curLine;
    int lineSpeed = linesModel->getLineSpeed(lineId);

    //size() - 1 to exclude AddHere
    for(int idx = firstIdx + 1; idx < stops.size() - 1; idx++)
    {
        StopItem& item = stops[idx];
        if(item.curLine != lineId)
        {
            lineId = item.curLine;
            lineSpeed = linesModel->getLineSpeed(lineId);
        }

        stationsToUpdate.insert(item.stationId);
        //TODO: should update also RS

        //Use original arrival to query train max speed

        //Temporarily set arrival and departure
        //Don't sync with database otherwise we could mess with stop order
        //Sync at end when we calculated all new times
        //If a previous stop gets postponed it appears after current stop
        //until we postpone also current stop. This temp reorder breaks the
        //query for retrieving train speed because this query is based on sorting by time

        //Get train speed
        q.bind(1, mJobId);
        q.bind(2, item.arrival);
        q.step();
        int speedKmH = q.getRows().get<int>(0);
        q.reset();

        //If line is slower or we couldn't get trains speed (likely no RS coupled) use line max speed instead
        if(speed <= 0 || (lineSpeed >= 0 && lineSpeed < speed))
            speed = lineSpeed;

        double distanceMeters = linesModel->getStationsDistance(lineId, prevStId, item.stationId);

        if(qFuzzyIsNull(distance) || speed < 1.0)
        {
            //Error
        }

        const double secs = (distanceMeters + accelerationDistMeters)/double(speed) * 3.6;
        int roundedTop = qCeil(secs) + offsetSecs;

        if(roundedTop < 60)
        {
            //At least 1 minute between stops
            roundedTop = 60;
        }
        else
        {
            //Align to minutes, we don't support seconds in train scheduled times
            int rem = roundedTop % 60;
            if(rem > 10) //TODO: maybe 20 as treshold, sync calcTimeBetweenStations()
                roundedTop += 60 - rem; //Round to next minute
            else
                roundedTop -= rem;
        }

        int stopTime = item.arrival.secsTo(item.departure);
        item.arrival = prevDep.addSecs(roundedTop);
        item.departure = prevDep.addSecs(roundedTop + stopTime);

        prevStId = item.stationId;
        prevDep = item.departure;
    }

    //Now prepare query to set Arrival and departure
    q.prepare("UPDATE stops SET arrival=?,departure=? WHERE id=?");

    for(int idx = firstIdx; idx < stops.size() - 1; idx++)
    {
        //Now sync with database

        const StopItem& item = stops.at(idx);
        q.bind(1, item.arrival);
        q.bind(2, item.departure);
        q.bind(3, item.stopId);
        q.step();
        q.reset();
    }

    //Finally inform the view
    QModelIndex first = index(firstIdx, 0);
    QModelIndex last = index(stops.size() - 1);
    emit dataChanged(first, last);
}
#endif

void StopModel::setStation_internal(StopItem& item, db_id stId, db_id nodeId)
{
    int platf = 0;
    //Check if platform is out of range
    query q_getDefPlatf(mDb, "SELECT defplatf_freight,defplatf_passenger FROM stations WHERE id=?");
    q_getDefPlatf.bind(1, stId);
    if(q_getDefPlatf.step() != SQLITE_ROW)
    {
        //Error
    }
    auto r = q_getDefPlatf.getRows();
    const int defPlatf_freight = r.get<int>(0);
    const int defPlatf_passenger = r.get<int>(1);

    platf = category >= FirstPassengerCategory ?
                defPlatf_passenger : defPlatf_freight;


    q_setPlatform.bind(1, platf);
    q_setPlatform.bind(2, item.stopId);
    q_setPlatform.execute();
    q_setPlatform.reset();
    item.platform = platf;

    //Mark for update both old and new station
    if(item.stationId)
        stationsToUpdate.insert(item.stationId);
    if(stId)
        stationsToUpdate.insert(stId);

    item.stationId = stId;

    q_setStopSt.bind(1, stId);
    if(nodeId)
        q_setStopSt.bind(2, nodeId);
    else
        q_setStopSt.bind(2, null_type{}); //Bind NULL instead of 0
    q_setStopSt.bind(3, item.stopId);
    q_setStopSt.execute();
    q_setStopSt.reset();
}

void StopModel::shiftStopsBy24hoursFrom(const QTime &startTime)
{
    //HACK: when applying msecOffset to stops query might not work because there might be a (next) stop with the same arrival
    //      that is being set and thus SQL hits UNIQUE constraint and rejects the modification
    //SOLUTION: shift all subsequent stops by 24 hours so there will be no conflicts and then reset the time once at a time
    //          so in the end they all will have correct time (no need to shift backwards)

    command q_shiftArrDep(mDb, "UPDATE stops SET arrival=arrival+?1,departure=departure+?1 WHERE job_id=?2 AND arrival>?3");
    const int shiftMin = 24 * 60; //Shift by 24h
    q_shiftArrDep.bind(1, shiftMin);
    q_shiftArrDep.bind(2, mJobId);
    q_shiftArrDep.bind(3, startTime);
    q_shiftArrDep.execute();
    q_shiftArrDep.finish();
}

void StopModel::setStation(const QPersistentModelIndex& idx, db_id stId)
{
    if(!idx.isValid() || idx.row() >= stops.count())
        return;

    StopItem& s = stops[idx.row()];
    if(s.stationId == stId)
        return;

    startStopsEditing();

    q_getRwNode.bind(1, s.segment);
    q_getRwNode.bind(2, stId);
    q_getRwNode.step();
    db_id nodeId = q_getRwNode.getRows().get<db_id>(0);
    q_getRwNode.reset();

    setStation_internal(s, stId, nodeId);

    emit dataChanged(idx, idx);

    //NOTE: Here 'idx' may change so we use QPersistentModelIndex
    if(autoInsertTransits)
    {
        insertTransitsBefore(idx);
    }

    //Calculate time after transits have been inserted in beetween
    if(timeCalcEnabled)
    {
        int prevIdx = idx.row() - 1;
        if(prevIdx >= 0)
        {
            const StopItem& prev = stops[prevIdx];
            QTime arrival = prev.departure;

            //Add travel duration (At least 60 secs)
            arrival = arrival.addSecs(qMax(60, calcTimeBetweenStInSecs(prev.stationId, stId, s.curLine)));
            int secs = arrival.second();
            if(secs > 10)
            {
                //Round seconds to next minute
                arrival = arrival.addSecs(60 - secs);
            }
            else
            {
                //Round seconds to previous minute
                arrival = arrival.addSecs(-secs);
            }

            setArrival(idx, arrival, true);
        }
    }
}

void StopModel::setPlatform(const QModelIndex& idx, int platf)
{
    if(!idx.isValid() || idx.row() >= stops.count())
        return;

    StopItem& s = stops[idx.row()];
    if(s.platform == platf)
        return;

    int platfCount = 0;
    int depotCount = 0;
    getStationPlatfCount(s.stationId, platfCount, depotCount);

    //Check if it's valid
    if(platf < 0)
    {
        //Depot platform
        if(platf < -depotCount)
            return; //Out of range
        //Note: only '>' because they start from '-1', '-2' and so on
        //Index 0 is a main platform
    }
    else
    {
        //Main platform
        if(platf >= platfCount)
            return; //Out of range
        //Note: '>=' because they start from '0', '1' and so on
    }

    startStopsEditing();

    //Mark the station for update
    stationsToUpdate.insert(s.stationId);

    s.platform = platf;
    q_setPlatform.bind(1, s.platform);
    q_setPlatform.bind(2, s.stopId);
    q_setPlatform.execute();
    q_setPlatform.reset();

    emit dataChanged(idx, idx);
}

bool StopModel::isAddHere(const QModelIndex &idx)
{
    if(idx.isValid() && idx.row() < stops.count())
        return (stops[idx.row()].addHere != 0);
    return false;
}

bool StopModel::updatePrevSegment(StopItem &prevStop, StopItem &curStop, db_id segmentId)
{
    bool reversed = false;
    query q(mDb, "SELECT s.in_gate_id,g1.station_id,s.out_gate_id,g2.station_id"
                 " FROM railway_segments s"
                 " JOIN station_gates g1 ON g1.id=s.in_gate_id"
                 " JOIN station_gates g2 ON g2.id=s.out_gate_id"
                 " WHERE s.id=?");
    q.bind(1, segmentId);
    if(q.step() != SQLITE_ROW)
        return false;

    auto seg = q.getRows();
    db_id in_gateId = seg.get<db_id>(0);
    db_id in_stationId = seg.get<db_id>(1);
    db_id out_gateId = seg.get<db_id>(2);
    db_id out_stationId = seg.get<db_id>(3);

    if(out_stationId == prevStop.stationId)
    {
        //Segment is reversed
        qSwap(in_gateId, out_gateId);
        qSwap(in_stationId, out_stationId);
        reversed = true;
    }
    else if(in_stationId != prevStop.stationId)
    {
        //Error: segment is not connected to previous station
        return false;
    }

    if(out_stationId != curStop.stationId)
    {
        //Error: segment is not connected to next (current) station
        return false;
    }

    if(in_gateId != prevStop.toGate.gateId)
    {
        //Try to find a gate connected to previous track_id
        QByteArray sql = "SELECT c.id,c.gate_track,sc.id,sc.%2_track"
                         " FROM station_gate_connections c"
                         " JOIN railway_connections sc ON sc.seg_id=?3 AND sc.%1_track=c.gate_track"
                         " WHERE c.track_id=?1 AND c.gate_id=?2";
        if(reversed)
        {
            sql.replace("%1", "out");
            sql.replace("%2", "in");
        }else{
            sql.replace("%1", "in");
            sql.replace("%2", "out");
        }

        q.prepare(sql);
        q.bind(1, prevStop.trackId);
        q.bind(2, in_gateId);
        q.bind(3, segmentId);
        if(q.step() != SQLITE_ROW)
        {
            //Error: gate is not connected to previous track
            //User must change previous track
            return false;
        }

        auto conn = q.getRows();
        prevStop.toGate.gateConnId = conn.get<db_id>(0);
        prevStop.toGate.gateId = in_gateId;
        prevStop.toGate.trackNum = conn.get<int>(1);
        prevStop.nextSegment.segConnId = conn.get<db_id>(2);
        prevStop.nextSegment.segmentId = segmentId;
        prevStop.nextSegment.outTrackNum = conn.get<int>(3);

        //Update prev stop
        command cmd(mDb, "UPDATE stops SET out_gate_conn=?, next_segment_conn_id=? WHERE id=?");
        cmd.bind(1, prevStop.toGate.gateConnId);
        cmd.bind(2, prevStop.nextSegment.segConnId);
        cmd.bind(3, prevStop.stopId);
        if(cmd.execute() != SQLITE_OK)
        {
            qWarning() << "StopModel: cannot update previous stop segment:" << mDb.error_msg() << prevStop.stopId;
            return false;
        }
    }

    curStop.fromGate.gateConnId = 0;
    curStop.fromGate.gateId = out_gateId;
    curStop.fromGate.trackNum = prevStop.nextSegment.outTrackNum;

    return true;
}

bool StopModel::updateCurrentInGate(StopItem &curStop, const StopItem::Segment &prevSeg)
{
    //FIIXME
    //query q(mDb, "SELECT ");
    return false;
}

void StopModel::setStopInfo(const QModelIndex &idx, StopItem newStop, StopItem::Segment prevSeg)
{
    const int row = idx.row();

    if(!idx.isValid() && row >= stops.count())
        return;

    StopItem& s = stops[row];

    if(s.type != First && row > 0)
    {
        //Update previous segment
        StopItem& prevStop = stops[row - 1];
        if(prevStop.nextSegment.segmentId != prevSeg.segmentId)
        {
            if(!updatePrevSegment(prevStop, newStop, prevSeg.segmentId))
                return;
            prevSeg = prevStop.nextSegment;
        }
    }

    command cmd(mDb);

    if(s.stationId != newStop.stationId)
    {
        //Update station, reset out gate
        cmd.prepare("UPDATE stops SET station_id=?,out_gate_conn=NULL WHERE id=?");
        cmd.bind(1, newStop.stationId);
        cmd.bind(2, s.stopId);

        if(cmd.execute() != SQLITE_OK)
            return;

        s.stationId = newStop.stationId;
        s.toGate.gateConnId = 0;
    }

    if(s.fromGate.gateConnId != newStop.fromGate.gateConnId)
    {
        updateCurrentInGate(newStop, prevSeg);
    }

    if(s.arrival != newStop.arrival || s.departure != newStop.departure)
    {
        //Update Arrival and Departure
        //NOTE: they must be set togheter so CHECK constraint fires at the end
        //Otherwise it would be impossible to set arrival > departure and then update departure

        cmd.prepare("UPDATE stops SET arrival=?,departure=? WHERE id=?");
        cmd.bind(1, newStop.arrival);
        cmd.bind(2, newStop.departure);
        cmd.bind(3, s.stopId);

        if(cmd.execute() != SQLITE_OK)
            return;

        s.arrival = newStop.arrival;
        s.departure = newStop.departure;
    }


}

void StopModel::removeStop(const QModelIndex &idx)
{
    const int row = idx.row();

    if(!idx.isValid() && row >= stops.count())
        return;

    StopItem& s = stops[row];
    if(s.addHere != 0)
        return;

    startStopsEditing();

    //Mark the station for update
    if(s.stationId)
        stationsToUpdate.insert(s.stationId);

    //BIG TODO: refactor code (Too many if/else) and emit dataChanged signal

    //Handle special cases: remove First or remove Last  BIG TODO

    if(stops.count() == 2) //First + AddHere
    {
        //Special case:
        //Remove First but we don't need to update next stops because there aren't
        //After this operation there is only the AddHere

        beginRemoveRows(QModelIndex(), row, row);

        deleteStop(s.stopId);
        destroySegment(s.segment, mJobId); //We were last stop so remove segment
        stops.removeAt(row);

        endRemoveRows();
        return;
    }

    if(s.type == First)
    {
        beginRemoveRows(QModelIndex(), row, row);

        QModelIndex nextIdx = index(row + 1, 0);
        StopItem& next = stops[nextIdx.row()];
        next.type = First;
        setDeparture(nextIdx, next.departure, true);

        if(next.nextLine != 0)
        {
            //There is a line change so this becomes First line
            //And we discard oldFirst line

            setStopSeg(next, next.nextSegment_);
            setNextSeg(next, 0); //Reset nextSeg
            next.curLine = 0;

            //nextLine stays as is because it's First

            //Destroy oldFirst segment because now it's empty
            destroySegment(s.segment, mJobId);
        }
        else
        {
            next.nextLine = next.curLine;
            next.curLine = 0; //Don't destroySegment
        }

        deleteStop(s.stopId);
        stops.removeAt(row);

        endRemoveRows();

        QModelIndex newFirstIdx = index(0, 0);
        emit dataChanged(newFirstIdx, newFirstIdx); //Update new First Stop
    }
    else if (s.type == Last)
    {
        QModelIndex prevIdx = index(row - 1, idx.column());
        StopItem& prev = stops[prevIdx.row()];

        if(autoMoveUncoupleToNewLast)
        {
            //We are new last stop and previous is not First (>= 1)
            //Move couplings from former last stop (now removed) to new last stop (former last but one)
            command q_moveUncoupled(mDb, "UPDATE OR IGNORE coupling SET stop_id=? WHERE stop_id=?");
            q_moveUncoupled.bind(1, prev.stopId);
            q_moveUncoupled.bind(2, s.stopId);
            int ret = q_moveUncoupled.execute();
            if(ret != SQLITE_OK)
            {
                qDebug() << "Error shifting uncoupling from stop:" << s.stopId << "to:" << prev.stopId << "Job:" << mJobId
                         << "err:" << ret << mDb.error_msg();

            }

            //Select them to update them
            query q_selectMoved(mDb, "SELECT rs_id FROM coupling WHERE stop_id=?");
            q_selectMoved.bind(1, prev.stopId);
            for(auto rs : q_selectMoved)
            {
                db_id rsId = rs.get<db_id>(0);
                rsToUpdate.insert(rsId);
            }
        }

        if(prev.type != First)
        {
            //Set previous stop to be Last
            //unless it's First: First remains of type Fisrt obviuosly

            //Reset Departure so it's equal to Arrival
            //Before setting stop type = Last, otherwise setDeparture doesn't work
            setDeparture(prevIdx, prev.arrival, false);
            setStopType(prevIdx, Last);
        }

        if(prev.nextLine == 0 || prev.type == First)
        {
            //If nextLine == 0 or previous is First
            //we just delete this stop leaving prev segment untouched.
            beginRemoveRows(QModelIndex(), row, row);
            deleteStop(s.stopId);
            stops.removeAt(row);
            endRemoveRows();
        }
        else
        {
            //Prev stop changes line so we are in a new segment.
            //When we delete this stop, the segment becomes useless so we destroy it.
            beginRemoveRows(QModelIndex(), row, row);

            db_id nextSeg = prev.nextSegment_;

            setNextSeg(prev, 0); //Reset nextSegment of prev stop
            prev.nextLine = 0;

            destroySegment(nextSeg, mJobId);
            deleteStop(s.stopId);
            stops.removeAt(row);

            endRemoveRows();
        }

        emit dataChanged(prevIdx, prevIdx); //Update previous stop
    }
    else
    {
        //BIG TODO: what if 'prev' is a transit???
        //Maybe we should go up to a non-transit stop?
        //Or set transit-linechange?

        //QModelIndex prevIdx = index(row - 1, 0);
        //StopItem& prev = stops[prevIdx.row()];
    }
}

void StopModel::insertStopBefore(const QModelIndex &idx)
{
    if(!idx.isValid() && idx.row() >= stops.count())
        return;

    const int row = idx.row();

    //Use a pointer because QVector may realloc so a reference would became invalid
    StopItem* s = &stops[row];
    if(s->addHere != 0)
        return;

    startStopsEditing();

    //Handle special case: insert before First  BIG TODO
    if(s->type == First)
    {
        beginInsertRows(QModelIndex(), 0, 0);
        stops.insert(0, StopItem());
        endInsertRows();

        const QModelIndex oldFirstIdx = index(1, 0);

        StopItem& oldFirst = stops[1];
        StopItem& first = stops[0];

        first.type = First;
        oldFirst.type = Normal;

        first.addHere = 0;
        first.arrival = oldFirst.arrival;

        first.segment = oldFirst.segment;

        first.nextLine = oldFirst.nextLine;
        oldFirst.curLine = first.nextLine;
        oldFirst.nextLine = 0;

        first.stopId = createStop(mJobId, first.segment, first.arrival);

        //Shift next stops by 1 minute
        setArrival(oldFirstIdx, first.arrival.addSecs(60), true);

        emit dataChanged(index(0, 0),
                         oldFirstIdx);
    }
    else
    {
        beginInsertRows(QModelIndex(), row, row);
        stops.insert(row, StopItem());
        endInsertRows();
        s = &stops[row + 1];
        StopItem& new_stop = stops[row];
        const StopItem& prev = stops[row - 1];

        const QTime arr = prev.departure.addSecs(60);
        new_stop.arrival = arr;
        new_stop.segment = s->segment;

        new_stop.stopId = createStop(mJobId, new_stop.segment, new_stop.arrival);
        new_stop.nextLine = 0;
        new_stop.curLine = s->curLine;
        new_stop.addHere = 0;
        new_stop.type = Normal;

        //Set stop time and shift next stops
        int secs = defaultStopTimeSec();

        if(secs == 0)
        {
            //If secs is 0, trasform stop in Transit
            setStopType(idx, Transit);
        }
        else
        {
            setDeparture(idx, arr.addSecs(secs), true);
        }

        emit dataChanged(idx,
                         idx);
    }
}

JobCategory StopModel::getCategory() const
{
    return category;
}

void StopModel::setCategory(int value)
{
    if(int(category) == value)
        return;

    startInfoEditing();

    category = JobCategory(value);
    emit categoryChanged(int(category));
}

bool StopModel::setNewJobId(db_id jobId)
{
    if(mNewJobId == jobId)
        return true;

    if(jobId != mJobId)
    {
        //If setting a different id than original, check if it's already existent
        query q_getJob(mDb, "SELECT 1 FROM jobs WHERE id=?");
        q_getJob.bind(1, jobId);
        int ret = q_getJob.step();
        if(ret == SQLITE_ROW)
        {
            //Already exists, revert back to previous job id
            emit jobIdChanged(mNewJobId);
            return false;
        }
    }

    //The new job id is valid
    startInfoEditing();
    mNewJobId = jobId;
    emit jobIdChanged(mNewJobId);
    return true;
}

db_id StopModel::getJobId() const
{
    return mJobId;
}

void StopModel::setNewShiftId(db_id shiftId)
{
    if(newShiftId == shiftId)
        return;

    if(stops.count() < 3) //First + Last + AddHere
    {
        emit errorSetShiftWithoutStops();
        return;
    }

    startInfoEditing();

    newShiftId = shiftId;

    emit jobShiftChanged(newShiftId);
}

//Called for example when changing a job's shift from the ShiftGraphEditor
void StopModel::onExternalShiftChange(db_id shiftId, db_id jobId)
{
    if(jobId == mJobId)
    {
        //Don't start stop/info editing because the change was already made by JobsModel
        //Prevent discarding the change by updating also original shift
        jobShiftId = newShiftId = shiftId;

        emit jobShiftChanged(jobShiftId);
    }
}

void StopModel::onShiftNameChanged(db_id shiftId)
{
    if(newShiftId == shiftId)
    {
        emit jobShiftChanged(newShiftId);
    }
}

void StopModel::onStationLineNameChanged()
{
    //Stations and line names are fetched by delegate while painting
    //We just need to repaint
    QModelIndex start = index(0, 0);
    QModelIndex end = index(stops.count(), 0);
    emit dataChanged(start, end);
}

bool StopModel::isEdited() const
{
    return editState != NotEditing;
}

db_id StopModel::getNewShiftId() const
{
    return newShiftId;
}

db_id StopModel::getJobShiftId() const
{
    return jobShiftId;
}

QString StopModel::getDescription(const StopItem& s) const
{
    if(s.addHere != 0)
        return QString();

    query q_getDescr(mDb, "SELECT description FROM stops WHERE id=?");
    q_getDescr.bind(1, s.stopId);
    q_getDescr.step();
    const QString descr = q_getDescr.getRows().get<QString>(0);
    q_getDescr.reset();

    return descr;
}

void StopModel::setDescription(const QModelIndex& idx, const QString& descr)
{
    if(!idx.isValid() && idx.row() >= stops.count())
        return;

    StopItem& s = stops[idx.row()];
    if(s.addHere != 0)
        return;

    startStopsEditing();

    //Mark the station for update
    stationsToUpdate.insert(s.stationId);

    command q_setDescr(mDb, "UPDATE stops SET description=? WHERE id=?");
    q_setDescr.bind(1, descr);
    q_setDescr.bind(2, s.stopId);
    q_setDescr.execute();
    q_setDescr.finish();

    emit dataChanged(idx, idx, {STOP_DESCR_ROLE});
}

void StopModel::setTimeCalcEnabled(bool value)
{
    timeCalcEnabled = value;
}

int StopModel::calcTimeBetweenStInSecs(db_id stA, db_id stB, db_id lineId)
{
    DEBUG_IMPORTANT_ENTRY;

    query q(mDb, "SELECT max_speed FROM lines WHERE id=?");
    q.bind(1, lineId);
    if(q.step() != SQLITE_ROW)
        return 0; //Error
    const double speedKmH = q.getRows().get<double>(0);

    const double meters = 0; //lines::getStationsDistanceInMeters(mDb, lineId, stA, stB);

    qDebug() << "Km:" << meters/1000.0 << "Speed:" << speedKmH;

    if(qFuzzyIsNull(meters) || speedKmH < 1.0)
        return 0; //Error

    const double secs = (meters + accelerationDistMeters)/speedKmH * 3.6;
    qDebug() << "Time:" << secs;
    return qCeil(secs);
}

int StopModel::defaultStopTimeSec()
{
    //TODO: the prefernces should be stored also in database
    return AppSettings.getDefaultStopMins(int(category)) * 60;
}

void StopModel::removeLastIfEmpty()
{
    if(stops.count() < 2) //Empty stop + AddHere TODO: count < 3
        return;

    int row = stops.count() - 2; //Last index (size - 1) is AddHere so we need 'size() - 2'

    //Recursively remove empty stops
    //GreaterEqual because we include First
    while (row >= 0)
    {
        const StopItem& stop = stops[row];

        if(stop.stationId == 0)
        {
            removeStop(index(row));
        }
        else
        {
            //This stop is valid so it will be the actual Last stop
            //(Or First if we removed all rows but first one)
            //So stop the loop
            break;
        }

        //Try with previous stop
        row--;
    }
}

std::pair<QTime, QTime> StopModel::getFirstLastTimes() const
{
    QTime first, last;

    if(stops.count() > 1) //First + AddHere
    {
        first = stops[0].departure;
    }

    int row = stops.count() - 2; //Last indx (size - 1) is AddHere so we need 'size() - 2'

    //GreaterEqual because we include First
    //Recursively skip last stop if empty
    while (row >= 0)
    {
        const StopItem& stop = stops[row];

        if(stop.stationId != 0)
        {
            last = stop.arrival;
            break;
        }

        //Try with previous stop
        row--;
    }

    return std::make_pair(first, last);
}

void StopModel::setAutoInsertTransits(bool value)
{
    autoInsertTransits = value;
}

void StopModel::setAutoMoveUncoupleToNewLast(bool value)
{
    autoMoveUncoupleToNewLast = value;
}

void StopModel::setAutoUncoupleAtLast(bool value)
{
    autoUncoupleAtLast = value;
}

void StopModel::insertTransitsBefore(const QPersistentModelIndex& stop)
{
    //TODO: should ask user when deleting transits to replace them
    //TODO: to change path user could also change 'NextLine' in previous

    if(stop.row() == 0) //It's First Stop, there is nothing before it.
        return;
    const StopItem to = stops.at(stop.row()); //Make a deep copy
    db_id prevStId = 0;
    QTime prevDep;

    //Find previous stop (ignore transits in beetween)
    int prevRow = stop.row();
    for(; prevRow >= 0; prevRow--)
    {
        StopType t = stops.at(prevRow).type;
        if(t == Normal || t == First || t == TransitLineChange)
        {
            prevStId = stops.at(prevRow).stationId;
            prevDep = stops.at(prevRow).departure;
            break;
        }
    }

    int oldLastTransitRow = stop.row() - 1;
    if(prevRow < oldLastTransitRow)
    {
        //Remove old transits in between
        beginRemoveRows(QModelIndex(), prevRow + 1, oldLastTransitRow);

        for(int i = prevRow + 1; i <= oldLastTransitRow; i++)
        {
            deleteStop(stops.at(i).stopId);
        }

        auto it = stops.begin() + prevRow + 1;
        auto end = stops.begin() + oldLastTransitRow;
        stops.erase(it, end + 1);

        endRemoveRows();
    }

    //Count transits first
    query q(mDb, "SELECT COUNT()"
                 " FROM railways r"
                 " JOIN railways fromSt ON fromSt.stationId=? AND fromSt.lineId=r.lineId"
                 " JOIN railways toSt ON toSt.stationId=? AND toSt.lineId=r.lineId"
                 " WHERE r.lineId=? AND"
                 " CASE WHEN fromSt.pos_meters < toSt.pos_meters"
                 " THEN (r.pos_meters < toSt.pos_meters AND r.pos_meters > fromSt.pos_meters)"
                 " ELSE (r.pos_meters > toSt.pos_meters AND r.pos_meters < fromSt.pos_meters)"
                 " END");
    q.bind(1, prevStId);
    q.bind(2, to.stationId);
    q.bind(3, to.curLine);
    q.step();
    int count = q.getRows().get<int>(0);
    q.finish();

    if(count == 0)
        return; //No transits to insert

    q.prepare("SELECT max_speed FROM lines WHERE id=?");
    q.bind(1, to.curLine);
    q.step();
    const double speedKmH = qMax(1.0, q.getRows().get<double>(0));
    q.finish();

    int curRow = prevRow + 1;
    beginInsertRows(QModelIndex(), curRow, curRow + count - 1);

    stops.insert(curRow, count, StopItem());

    q.prepare("SELECT r.id, r.pos_meters, r.stationId, fromSt.pos_meters"
              " FROM railways r"
              " JOIN railways fromSt ON fromSt.stationId=? AND fromSt.lineId=r.lineId"
              " JOIN railways toSt ON toSt.stationId=? AND toSt.lineId=r.lineId"
              " WHERE r.lineId=? AND"
              " CASE WHEN fromSt.pos_meters < toSt.pos_meters"
              " THEN (r.pos_meters < toSt.pos_meters AND r.pos_meters > fromSt.pos_meters)"
              " ELSE (r.pos_meters > toSt.pos_meters AND r.pos_meters < fromSt.pos_meters)"
              " END"
              " ORDER BY"
              " CASE WHEN fromSt.pos_meters > toSt.pos_meters THEN r.pos_meters END DESC,"
              " CASE WHEN fromSt.pos_meters < toSt.pos_meters THEN r.pos_meters END ASC");

    q.bind(1, prevStId);
    q.bind(2, to.stationId);
    q.bind(3, to.curLine);

    auto it = q.begin();

    int oldKmMeters = (*it).get<int>(3);

    autoInsertTransits = false; //Prevent recursion from setStaton()
    for(; it != q.end(); ++it)
    {
        auto r = *it;
        db_id nodeId = r.get<db_id>(0);
        int kmInMeters = r.get<int>(1);
        db_id stId = r.get<db_id>(2);

        //qDebug() << "Km:" << km << "St:" << stationsModel->getStName(stId) << stId;

        if(timeCalcEnabled)
        {
            int distanceMeters = qAbs(oldKmMeters - kmInMeters);
            //Add travel duration (At least 60 secs)
            const double secs = (distanceMeters + accelerationDistMeters)/speedKmH * 3.6;
            prevDep = prevDep.addSecs(qMax(60, qCeil(secs)));

            int reminder = prevDep.second();
            if(reminder > 10)
            {
                //Round seconds to next minute
                prevDep = prevDep.addSecs(60 - reminder);
            }
            else
            {
                //Round seconds to previous minute
                prevDep = prevDep.addSecs(-reminder);
            }

        }
        else
        {
            prevDep = prevDep.addSecs(60);
        }

        oldKmMeters = kmInMeters;

        StopItem &item = stops[curRow];
        item.type = Transit;
        item.addHere = 0;
        item.stationId = 0;
        item.curLine = to.curLine;
        item.segment = to.segment;
        item.nextLine = 0;
        item.nextSegment_ = 0;
        item.platform = 0;
        item.arrival = item.departure = prevDep;
        item.stopId = createStop(mJobId, item.segment, item.arrival, Transit);

        setStation_internal(item, stId, nodeId);

        curRow++;
    }
    q.finish();
    autoInsertTransits = true; //Re-enable flag

    endInsertRows();

    qDebug() << "End";
}

bool StopModel::startInfoEditing() //Faster than 'startStopEditing()' use this if change doesen.t affect stops
{
    if(editState != NotEditing)
        return true;

    if(!mJobId)
        return false;

    editState = InfoEditing;
    emit edited(true);

    return true;
}


//NOTE: this must be called before any edit to database in order to save previous state
bool StopModel::startStopsEditing()
{
    if(editState == StopsEditing)
        return true;

    if(!mJobId)
        return false;

    bool alreadyEditing = editState == InfoEditing;
    editState = StopsEditing;

    //Backup stops
    command q_backup(mDb, "INSERT INTO old_stops SELECT * FROM stops WHERE job_id=?");
    q_backup.bind(1, mJobId);
    int ret = q_backup.execute();
    q_backup.reset();

    if(ret != SQLITE_OK)
    {
        qDebug() << "Error while saving old stops:" << ret << mDb.error_msg() << mDb.extended_error_code();
        return false;
    }

    //Backup couplings
    q_backup.prepare("INSERT INTO old_coupling(id, stop_id, rs_id, operation)"
                     " SELECT coupling.id, coupling.stop_id, coupling.rs_id, coupling.operation"
                     " FROM coupling"
                     " JOIN stops ON stops.id=coupling.stop_id WHERE stops.job_id=?");
    q_backup.bind(1, mJobId);
    ret = q_backup.execute();
    q_backup.reset();

    if(ret != SQLITE_OK)
    {
        qDebug() << "Error while saving old couplings:" << ret << mDb.error_msg() << mDb.extended_error_code();
        return false;
    }

    if(!alreadyEditing)
        emit edited(true);

    return true;
}

bool StopModel::endStopsEditing()
{
    if(editState == NotEditing)
        return false;

    if(editState == StopsEditing)
    {
        //Clear old_stops (will automatically clear old_couplings with FK: ON DELETE CASCADE)
        command q_clearOldStops(mDb, "DELETE FROM old_stops WHERE job_id=?");
        q_clearOldStops.bind(1, mJobId);
        int ret = q_clearOldStops.execute();
        q_clearOldStops.reset();

        if(ret != SQLITE_OK)
        {
            qDebug() << "Error while clearing old stops:" << ret << mDb.error_msg() << mDb.extended_error_code();
            return false;
        }
    }

    editState = NotEditing;

    emit edited(false);

    return true;
}

bool StopModel::commitChanges()
{
    if(editState == NotEditing)
        return true;

    command q(mDb, "UPDATE jobs SET category=?, shift_id=? WHERE id=?");
    q.bind(1, int(category));
    if(newShiftId)
        q.bind(2, newShiftId);
    else
        q.bind(2); //NULL shift
    q.bind(3, mJobId);
    if(q.execute() != SQLITE_OK)
    {
        category = oldCategory;
        emit categoryChanged(int(category));
        newShiftId = jobShiftId;
        emit jobShiftChanged(newShiftId);
    }

    db_id oldJobId = mJobId;

    if(mNewJobId != mJobId)
    {
        q.prepare("UPDATE jobs SET id=? WHERE id=?");
        q.bind(1, mNewJobId);
        q.bind(2, mJobId);
        int ret = q.execute();
        if(ret == SQLITE_OK)
        {
            mJobId = mNewJobId;
        }
        else
        {
            //Reset to old value
            mNewJobId = oldJobId;
            emit jobIdChanged(mJobId);
        }
    }

    oldCategory = category;

    emit Session->shiftJobsChanged(jobShiftId, oldJobId);
    emit Session->shiftJobsChanged(newShiftId, mNewJobId);
    jobShiftId = newShiftId;

    emit Session->jobChanged(mNewJobId, oldJobId);

    rsToUpdate.clear();
    stationsToUpdate.clear();

    return endStopsEditing();
}

bool StopModel::revertChanges()
{
    if(editState == NotEditing)
        return true;

    bool needsStopReload = false;

    if(editState == StopsEditing)
    {
        //Delete current data

        //Clear stops (will automatically clear couplings with FK: ON DELETE CASCADE)
        command cmd(mDb, "DELETE FROM stops WHERE job_id=?");
        cmd.bind(1, mJobId);
        int ret = cmd.execute();
        cmd.finish();

        if(ret != SQLITE_OK)
        {
            qDebug() << "Error while clearing current stops:" << ret << mDb.error_msg() << mDb.extended_error_code();
            return false;
        }

        //Now restore old data

        //Restore stops
        cmd.prepare("INSERT INTO stops SELECT * FROM old_stops WHERE job_id=?");
        cmd.bind(1, mJobId);
        ret = cmd.execute();
        cmd.reset();

        if(ret != SQLITE_OK)
        {
            qDebug() << "Error while restoring old stops:" << ret << mDb.error_msg() << mDb.extended_error_code();
            return false;
        }

        //Restore couplings
        cmd.prepare("INSERT INTO coupling(id, stop_id, rs_id, operation)"
                    " SELECT old_coupling.id, old_coupling.stop_id, old_coupling.rs_id, old_coupling.operation"
                    " FROM old_coupling"
                    " JOIN stops ON stops.id=old_coupling.stop_id WHERE stops.job_id=?");
        cmd.bind(1, mJobId);
        ret = cmd.execute();
        cmd.reset();

        if(ret != SQLITE_OK)
        {
            qDebug() << "Error while restoring old couplings:" << ret << mDb.error_msg() << mDb.extended_error_code();
            return false;
        }

        //Reload info and stops
        needsStopReload = true;
    }
    else
    {
        //Just reset info in case of InfoEditing
        mNewJobId = mJobId;
        emit jobIdChanged(mJobId);

        category = oldCategory;
        emit categoryChanged(int(category));

        newShiftId = jobShiftId;
        emit jobShiftChanged(jobShiftId);

        rsToUpdate.clear();
        stationsToUpdate.clear();
    }

    bool ret = endStopsEditing();
    if(!ret)
        return ret;

    if(needsStopReload)
        loadJobStops(mJobId);

    return true;
}

int StopModel::getStopRow(db_id stopId) const
{
    for(int row = 0; row < stops.size(); row++)
    {
        if(stops.at(row).stopId == stopId)
            return row;
    }

    return -1;
}
