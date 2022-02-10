#include "stopmodel.h"

#include <QDebug>

#include "app/scopedebug.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "stations/station_utils.h"

#include <QtMath>

StopModel::StopModel(database &db, QObject *parent) :
    QAbstractListModel(parent),
    mDb(db),
    mJobId(0),
    mNewJobId(0),
    jobShiftId(0),
    newShiftId(0),
    category(JobCategory::FREIGHT),
    oldCategory(JobCategory::FREIGHT),
    editState(NotEditing),
    timeCalcEnabled(true),
    autoInsertTransits(false),
    autoMoveUncoupleToNewLast(true),
    autoUncoupleAtLast(true)
{
    reloadSettings();
    connect(&AppSettings, &MRTPSettings::stopOptionsChanged, this, &StopModel::reloadSettings);

    connect(Session, &MeetingSession::shiftJobsChanged, this, &StopModel::onExternalShiftChange);
    connect(Session, &MeetingSession::shiftNameChanged, this, &StopModel::onShiftNameChanged);

    connect(Session, &MeetingSession::segmentNameChanged, this, &StopModel::onStationSegmentNameChanged);
    connect(Session, &MeetingSession::stationNameChanged, this, &StopModel::onStationSegmentNameChanged);
}

QVariant StopModel::data(const QModelIndex &index, int role) const
{
    return QVariant(); //Use setters and getters instead of data() and setData()
}

int StopModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : stops.count();
}

Qt::ItemFlags StopModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

void StopModel::setTimeCalcEnabled(bool value)
{
    timeCalcEnabled = value;
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
                          "stops.in_gate_conn, g1.gate_id, g1.gate_track, g1.track_id, g1.track_side,"
                          "stops.out_gate_conn, g2.gate_id, g2.gate_track, g2.track_id, g2.track_side,"
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
        s.fromGate.stationTrackSide = utils::Side(stop.get<int>(9));

        s.toGate.gateConnId = stop.get<db_id>(10);
        s.toGate.gateId = stop.get<db_id>(11);
        s.toGate.trackNum = stop.get<int>(12);
        db_id otherTrackId = stop.get<db_id>(13);
        s.toGate.stationTrackSide = utils::Side(stop.get<int>(14));

        s.nextSegment.segConnId = stop.get<db_id>(15);
        s.nextSegment.segmentId = stop.get<db_id>(16);
        s.nextSegment.inTrackNum = stop.get<db_id>(17);
        s.nextSegment.outTrackNum = stop.get<db_id>(18);

        db_id segInGateId = stop.get<db_id>(19);
        db_id segOutGateId = stop.get<db_id>(20);

        if(s.toGate.gateId && s.toGate.gateId == segOutGateId)
        {
            //Segment is reversed
            qSwap(segInGateId, segOutGateId);
            qSwap(s.nextSegment.inTrackNum, s.nextSegment.outTrackNum);
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

            if(s.toGate.trackNum != s.nextSegment.inTrackNum)
            {
                //Out gate leads to a different than next segment in track
                qWarning() << "Stop:" << s.stopId << "Different out gate track:" << s.toGate.gateConnId << s.nextSegment.segConnId;
            }
        }

        prevSegment = s.nextSegment;
        prevOutGateId = segOutGateId;

        s.type = StopType::Normal;

        if(i == 0)
        {
            s.type = StopType::First;
            if(stopType != 0)
            {
                //Error First cannot be a transit
                qWarning() << "Error: First stop cannot be transit! Job:" << mJobId << "StopId:" << s.stopId;
            }
        }
        else if (stopType)
        {
            s.type = StopType::Transit;

            if(i == count - 1)
            {
                //Error Last cannot be a transit
                qWarning() << "Error: Last stop cannot be transit! Job:" << mJobId << "StopId:" << s.stopId;
            }
        }

        stops.append(s);
        i++;
    }
    q_selectStops.finish();

    if(!stops.isEmpty() && stops.last().type != StopType::First)
        stops.last().type = StopType::Last; //Update Last stop type unless it's First

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

bool StopModel::isEdited() const
{
    return editState != NotEditing;
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

    if(jobShiftId != newShiftId)
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
        //StopsEditing triggers stop loading so already resets info

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
        last.type = StopType::Last;

        StopItem& s = stops[prevIdx];
        if(prevIdx > 0)
        {
            //Update stop time and type
            s.type = StopType::Normal;

            int secs = defaultStopTimeSec();
            if(secs == 0)
                s.type = StopType::Transit;

            s.departure = s.arrival.addSecs(secs);

            command cmd(mDb, "UPDATE stops SET departure=?,type=? WHERE id=?");
            cmd.bind(1, s.departure);
            cmd.bind(2, int(s.type));
            cmd.bind(3, s.stopId);
            cmd.execute();
        }

        /* Next stop must be at least one minute after
         * This is to prevent contemporary stops that will break ORDER BY arrival queries */

        int travelSecs = 60;
        if(s.nextSegment.segmentId)
            travelSecs = calcTravelTime(s.nextSegment.segmentId);

        const QTime time = s.departure.addSecs(travelSecs);
        last.arrival = time;
        last.departure = last.arrival;
        last.stopId = createStop(mJobId, last.arrival, last.departure, last.type);

        //Set station if previous stop has a next segment selected
        if(s.nextSegment.segConnId)
            updateCurrentInGate(last, s.nextSegment);

        if(autoMoveUncoupleToNewLast)
        {
            if(prevIdx >= 1)
            {
                //We are new last stop and previous is not First (>= 1)
                //Move uncoupled rs from former last stop (now last but one) to new last stop
                command q_moveUncoupled(mDb, "UPDATE OR IGNORE coupling SET stop_id=? WHERE stop_id=? AND operation=?");
                q_moveUncoupled.bind(1, last.stopId);
                q_moveUncoupled.bind(2, s.stopId);
                q_moveUncoupled.bind(3, int(RsOp::Uncoupled));
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
        last.type = StopType::First;

        last.arrival = QTime(0, 0);
        last.departure = last.arrival;
        last.stopId = createStop(mJobId, last.arrival, last.departure, last.type);
    }

    emit dataChanged(index(idx, 0),
                     index(idx, 0));

    insertAddHere(idx + 1, 1); //Insert only at end because may invalidate StopItem& references due to mem realloc
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

    if(stops.count() == 2) //First + AddHere
    {
        //Special case:
        //Remove First but we don't need to update next stops because there aren't
        //After this operation there is only the AddHere

        beginRemoveRows(QModelIndex(), row, row);

        deleteStop(s.stopId);
        stops.removeAt(row);

        endRemoveRows();
        return;
    }

    if(s.type == StopType::First)
    {
        beginRemoveRows(QModelIndex(), row, row);

        QModelIndex nextIdx = index(row + 1, 0);
        StopItem& next = stops[nextIdx.row()];
        next.type = StopType::First;
        const QTime oldArr = next.arrival;
        const QTime oldDep = next.arrival;
        updateStopTime(next, row + 1, true, oldArr, oldDep);

        //Set type to Normal
        command cmd(mDb, "UPDATE stops SET type=? WHERE id=?");
        cmd.bind(1, int(StopType::Normal));
        cmd.bind(2, next.stopId);
        cmd.execute();

        deleteStop(s.stopId);
        stops.removeAt(row);

        endRemoveRows();

        QModelIndex newFirstIdx = index(0, 0);
        emit dataChanged(newFirstIdx, newFirstIdx); //Update new First Stop
    }
    else if (s.type == StopType::Last)
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

        //Clear previous 'out' gate and next segment, set type to Normal, reset Departure so it's equal to Arrival
        command cmd(mDb, "UPDATE stops SET out_gate_conn=NULL,next_segment_conn_id=NULL,type=?,departure=arrival WHERE id=?");
        cmd.bind(1, int(StopType::Normal));
        cmd.bind(2, prev.stopId);
        cmd.execute();

        prev.toGate = StopItem::Gate{};
        prev.nextSegment = StopItem::Segment{};
        prev.departure = prev.arrival;

        //Set previous stop to be Last
        //unless it's First: First remains of type Fisrt obviuosly
        if(prev.type != StopType::First)
            prev.type = StopType::Last;

        beginRemoveRows(QModelIndex(), row, row);
        deleteStop(s.stopId);
        stops.removeAt(row);
        endRemoveRows();

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

void StopModel::removeLastIfEmpty()
{
    if(stops.count() < 2) //Empty stop + AddHere
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

void StopModel::uncoupleStillCoupledAtLastStop()
{
    if(!autoUncoupleAtLast)
        return;

    for(int i = stops.size() - 1; i >= 0; i--)
    {
        const StopItem& s = stops.at(i);
        if(s.addHere != 0 || !s.stationId)
            continue;

        //Found last stop with a station set
        uncoupleStillCoupledAtStop(s);
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

JobCategory StopModel::getCategory() const
{
    return category;
}

db_id StopModel::getJobId() const
{
    return mJobId;
}

db_id StopModel::getNewShiftId() const
{
    return newShiftId;
}

db_id StopModel::getJobShiftId() const
{
    return jobShiftId;
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

void StopModel::setStopInfo(const QModelIndex &idx, StopItem newStop, StopItem::Segment prevSeg)
{
    const int row = idx.row();
    int lastUpdatedRow = row;

    if(!idx.isValid() && row >= stops.count())
        return;

    command cmd(mDb);

    StopItem& s = stops[row];

    stationsToUpdate.insert(s.stationId);
    stationsToUpdate.insert(newStop.stationId);

    if(s.type != StopType::First && row > 0)
    {
        //Update previous segment
        StopItem& prevStop = stops[row - 1];
        if(prevStop.nextSegment.segmentId != prevSeg.segmentId)
        {
            if(!trySelectNextSegment(prevStop, prevSeg.segmentId, prevStop.toGate.trackNum,
                                      newStop.stationId, newStop.fromGate.gateId))
                return;

            startStopsEditing();

            //Update prev stop
            cmd.prepare("UPDATE stops SET out_gate_conn=?, next_segment_conn_id=? WHERE id=?");
            cmd.bind(1, prevStop.toGate.gateConnId);
            cmd.bind(2, prevStop.nextSegment.segConnId);
            cmd.bind(3, prevStop.stopId);
            if(cmd.execute() != SQLITE_OK)
            {
                qWarning() << "StopModel: cannot update previous stop segment:" << mDb.error_msg() << prevStop.stopId;
                return;
            }

            prevSeg = prevStop.nextSegment;
            newStop.fromGate.gateConnId = 0;
            newStop.fromGate.trackNum = prevStop.nextSegment.outTrackNum;
        }
    }

    if(s.stationId != newStop.stationId)
    {
        startStopsEditing();

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
        startStopsEditing();

        updateCurrentInGate(newStop, prevSeg);
        s.fromGate = newStop.fromGate;
        s.trackId = newStop.trackId;
    }

    if(s.arrival != newStop.arrival || s.departure != newStop.departure)
    {
        startStopsEditing();

        if(updateStopTime(newStop, row, true, s.arrival, s.departure))
        {
            //Succeded, store new sanitized values
            s.arrival = newStop.arrival;
            s.departure = newStop.departure;
            lastUpdatedRow = stops.count() - 2;
        }
    }

    const db_id oldSegConnId = s.nextSegment.segConnId;
    if(s.toGate.gateConnId != newStop.toGate.gateConnId)
    {
        startStopsEditing();

        //Update next stop
        cmd.prepare("UPDATE stops SET out_gate_conn=?, next_segment_conn_id=? WHERE id=?");
        cmd.bind(1, newStop.toGate.gateConnId);
        cmd.bind(2, newStop.nextSegment.segConnId);
        cmd.bind(3, newStop.stopId);
        if(cmd.execute() != SQLITE_OK)
        {
            qWarning() << "StopModel: cannot update previous stop segment:" << mDb.error_msg() << s.stopId;
            return;
        }

        s.toGate = newStop.toGate;
        s.nextSegment = newStop.nextSegment;

        if(s.type == StopType::First && s.toGate.gateConnId)
        {
            //No need to fake an in gate to set station track, we already have out gate
            cmd.prepare("UPDATE stops SET in_gate_conn=NULL WHERE id=?");
            cmd.bind(1, s.stopId);
            cmd.execute();
            s.fromGate = StopItem::Gate();
        }
    }

    if(row < stops.count() - 2 && s.nextSegment.segConnId && s.nextSegment.segConnId != oldSegConnId)
    {
        startStopsEditing();

        //Before Last and AddHere, so there is a stop after this
        //Update next station because "next" segment of previous (current) stop changed
        StopItem& nextStop = stops[row + 1];
        nextStop.fromGate.gateConnId = 0; //Reset to trigger update
        if(!updateCurrentInGate(nextStop, s.nextSegment))
            return;

        if(lastUpdatedRow == row)
            lastUpdatedRow++; //We updated also next row

        if(timeCalcEnabled)
        {
            const int secs = calcTravelTime(s.nextSegment.segmentId);
            const QTime oldNextArr = nextStop.arrival;
            const QTime oldNextDep = nextStop.departure;

            nextStop.arrival = s.departure.addSecs(secs);

            if(oldNextArr != nextStop.arrival)
            {
                if(!updateStopTime(nextStop, row + 1, true, oldNextArr, oldNextDep))
                {
                    //Failed, Reset to old values
                    nextStop.arrival = oldNextArr;
                    nextStop.departure = oldNextDep;
                }

                lastUpdatedRow = stops.count() - 2;
            }
        }
    }

    //Tell view to update
    const QModelIndex firstIdx = index(row, 0);
    const QModelIndex lastIdx = index(lastUpdatedRow, 0);
    emit dataChanged(firstIdx, lastIdx);
}

bool StopModel::setStopTypeRange(int firstRow, int lastRow, StopType type)
{
    if(firstRow < 0 || firstRow > lastRow || lastRow >= stops.count())
        return false;

    if(type == StopType::First || type == StopType::Last)
        return false;

    int defaultStopMsec = qMax(60, defaultStopTimeSec()) * 1000; //At least 1 minute

    StopType destType = type;

    startStopsEditing();
    shiftStopsBy24hoursFrom(stops.at(firstRow).arrival);

    query q_getCoupled(mDb, "SELECT rs_id, operation FROM coupling WHERE stop_id=?");
    command cmd(mDb, "UPDATE stops SET arrival=?,departure=?,type=? WHERE id=?");

    int msecOffset = 0;

    for(int r = firstRow; r <= lastRow; r++)
    {
        StopItem& s = stops[r];

        if(s.type == StopType::First || s.type == StopType::Last)
        {
            qWarning() << "Error: tried change type of First/Last stop:" << r << s.stopId << "Job:" << mJobId;

            //Always update time even if msecOffset == 0, because they have been shifted
            s.arrival = s.arrival.addMSecs(msecOffset);
            s.departure = s.departure.addMSecs(msecOffset);
            cmd.bind(1, s.arrival);
            cmd.bind(2, s.departure);
            cmd.bind(3, int(StopType::Normal));
            cmd.bind(4, s.stopId);
            cmd.execute();
            cmd.reset();
            continue;
        }

        if(type == StopType::ToggleType)
        {
            if(s.type == StopType::Normal)
                destType = StopType::Transit;
            else
                destType = StopType::Normal;
        }

        //Cannot couple or uncouple in transits
        if(destType == StopType::Transit)
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
                return false;
            }
        }

        //Mark the station for update
        stationsToUpdate.insert(s.stationId);

        s.arrival = s.arrival.addMSecs(msecOffset);
        s.departure = s.departure.addMSecs(msecOffset);

        if(destType == StopType::Normal)
        {
            s.type = StopType::Normal;

            if(s.arrival == s.departure)
            {
                msecOffset += defaultStopMsec;
                s.departure = s.arrival.addMSecs(defaultStopMsec);
            }
        }
        else
        {
            s.type = StopType::Transit;
            //Transit don't stop so departure is the same of arrival -> stop time = 0 minutes
            msecOffset -= s.arrival.msecsTo(s.departure);
            s.departure = s.arrival;
        }
        cmd.bind(1, s.arrival);
        cmd.bind(2, s.departure);
        cmd.bind(3, int(destType));
        cmd.bind(4, s.stopId);
        cmd.execute();
        cmd.reset();
    }

    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(stops.count() - 1, 0);

    //Always update time even if msecOffset == 0, because they have been shifted
    for(int r = lastRow + 1; r < stops.count(); r++)
    {
        StopItem& s = stops[r];
        destType = s.type;
        if(s.type == StopType::First || s.type == StopType::Last)
            destType = StopType::Normal;

        s.arrival = s.arrival.addMSecs(msecOffset);
        s.departure = s.departure.addMSecs(msecOffset);
        cmd.bind(1, s.arrival);
        cmd.bind(2, s.departure);
        cmd.bind(3, int(destType));
        cmd.bind(4, s.stopId);
        cmd.execute();
        cmd.reset();
    }

    emit dataChanged(firstIdx, lastIdx);

    return true;
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

    emit dataChanged(idx, idx);
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

bool StopModel::isAddHere(const QModelIndex &idx)
{
    if(idx.isValid() && idx.row() < stops.count())
        return (stops[idx.row()].addHere != 0);
    return false;
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

const QSet<db_id> &StopModel::getRsToUpdate() const
{
    return rsToUpdate;
}

const QSet<db_id> &StopModel::getStationsToUpdate() const
{
    return stationsToUpdate;
}

bool StopModel::isRailwayElectrifiedAfterStop(db_id stopId) const
{
    int row = getStopRow(stopId);
    if(row == -1)
        return true; //Error

    return isRailwayElectrifiedAfterRow(row);
}

bool StopModel::isRailwayElectrifiedAfterRow(int row) const
{
    if(row < 0 || row >= stops.count())
        return true; //Error

    const StopItem& item = stops.at(row);
    if(!item.nextSegment.segmentId)
        return true; //Error

    query q(mDb, "SELECT type FROM railway_segments WHERE id=?");
    q.bind(1, item.nextSegment.segmentId);
    if(q.step() != SQLITE_ROW)
        return true; //Error

    QFlags<utils::RailwaySegmentType> type = utils::RailwaySegmentType(q.getRows().get<int>(0));
    return type.testFlag(utils::RailwaySegmentType::Electrified);
}

bool StopModel::trySelectTrackForStop(StopItem &item)
{
    query q(mDb);
    if(item.fromGate.gateId)
    {
        //TODO: choose 'default' track, corretto tracciato
        //Try to keep previous selected station track otherwise choose lowest position track possible
        q.prepare("SELECT c.id, c.track_side, t.id"
                  " FROM station_gate_connections c"
                  " JOIN station_tracks t ON c.track_id=t.id"
                  " WHERE c.gate_id=? AND c.gate_track=?"
                  " ORDER BY c.track_id=? DESC, t.pos ASC"
                  " LIMIT 1");
        q.bind(1, item.fromGate.gateId);
        q.bind(2, item.fromGate.trackNum);
        q.bind(3, item.trackId);
        if(q.step() != SQLITE_ROW)
            return false;

        auto gate = q.getRows();
        item.fromGate.gateConnId = gate.get<db_id>(0);
        item.fromGate.stationTrackSide = utils::Side(gate.get<int>(1));
        item.trackId = gate.get<db_id>(2);
        return true;
    }

    //Select a random gate for first use
    q.prepare("SELECT c.id, c.gate_id, c.gate_track, c.track_side, t.id, MIN(t.pos)"
              " FROM station_tracks t"
              " JOIN station_gate_connections c ON c.track_id=t.id"
              " WHERE t.station_id=?"
              " LIMIT 1");
    q.bind(1, item.stationId);
    if(q.step() != SQLITE_ROW || q.getRows().column_type(0) == SQLITE_NULL)
        return false;

    auto gate = q.getRows();
    item.fromGate.gateConnId = gate.get<db_id>(0);
    item.fromGate.gateId = gate.get<db_id>(1);
    item.fromGate.trackNum = gate.get<int>(2);
    item.fromGate.stationTrackSide = utils::Side(gate.get<int>(3));
    item.trackId = gate.get<db_id>(4);

    //TODO: should we reset out gate here?

    return true;
}

bool StopModel::trySetTrackConnections(StopItem &item, db_id trackId, QString *outErr)
{
    query q(mDb, "SELECT station_id FROM station_tracks WHERE id=?");
    q.bind(1, trackId);
    if(q.step() == SQLITE_ROW)
    {
        db_id stId = q.getRows().get<db_id>(0);
        if(item.stationId != stId)
        {
            if(outErr)
                *outErr = tr("Track belongs to a different station.");
            return false;
        }
    }else{
        if(outErr)
            *outErr = tr("Track doesn't exist.");
        return false;
    }

    if(item.type == StopType::First)
    {
        //Fake in gate, select one just to set the track
        q.prepare("SELECT id,gate_id,gate_track,track_side FROM station_gate_connections WHERE track_id=? LIMIT 1");
        q.bind(1, trackId);
        if(q.step() != SQLITE_ROW)
        {
            if(outErr)
                *outErr = tr("Track is not connected to any of station gates.");
            return false;
        }

        auto gate = q.getRows();
        item.fromGate.gateConnId = gate.get<db_id>(0);
        item.fromGate.gateId = gate.get<db_id>(1);
        item.fromGate.trackNum = gate.get<int>(2);
        item.fromGate.stationTrackSide = utils::Side(gate.get<int>(3));
    }

    q.prepare("SELECT id, track_side FROM station_gate_connections"
              " WHERE gate_id=? AND gate_track=? AND track_id=?");

    if(item.type != StopType::First)
    {
        //Item is not First stop, check in gate
        q.bind(1, item.fromGate.gateId);
        q.bind(2, item.fromGate.trackNum);
        q.bind(3, trackId);

        if(q.step() == SQLITE_ROW)
        {
            //Found a connection
            item.fromGate.gateConnId = q.getRows().get<db_id>(0);
            item.fromGate.stationTrackSide = utils::Side(q.getRows().get<int>(1));
        }else{
            if(outErr)
                *outErr = tr("Track is not connected to in gate track.\n"
                             "Please choose a new track or change previous segment.");
            return false;
        }
        q.reset();
    }

    if(item.toGate.gateConnId)
    {
        //For every type of stop, check out gate
        //User already chose an out gate but now changed station track
        //Check if they are connected together
        q.bind(1, item.toGate.gateId);
        q.bind(2, item.toGate.trackNum);
        q.bind(3, trackId);

        if(q.step() == SQLITE_ROW)
        {
            //Found a connection
            item.toGate.gateConnId = q.getRows().get<db_id>(0);
            item.toGate.stationTrackSide = utils::Side(q.getRows().get<int>(1));
        }else{
            //Connection not found, inform user and reset out gate
            item.toGate = StopItem::Gate{};
            item.nextSegment = StopItem::Segment{}; //Reset next segment

            if(outErr)
                *outErr = tr("Track is not connected to selected out gate track.\n"
                             "Please choose a new out gate or out track, this might change next segment.");
            //Still return true to let user change out gate
        }
    }

    //Store new track ID
    item.trackId = trackId;

    return true;
}

bool StopModel::trySelectNextSegment(StopItem &item, db_id segmentId, int suggestedOutGateTrk, db_id nextStationId, db_id &seg_out_gateId)
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
    db_id seg_in_gateId = seg.get<db_id>(0);
    db_id in_stationId = seg.get<db_id>(1);
    seg_out_gateId = seg.get<db_id>(2);
    db_id out_stationId = seg.get<db_id>(3);

    if(out_stationId == item.stationId)
    {
        //Segment is reversed
        qSwap(seg_in_gateId, seg_out_gateId);
        qSwap(in_stationId, out_stationId);
        reversed = true;
    }
    else if(in_stationId != item.stationId)
    {
        //Error: segment is not connected to previous station
        return false;
    }

    if(nextStationId && out_stationId != nextStationId)
    {
        //Error: segment is not connected to next (current) station
        return false;
    }

    //Station out gate = segment in gate
    if(seg_in_gateId != item.toGate.gateId || item.toGate.trackNum != item.nextSegment.inTrackNum)
    {
        //Try to find a gate connected to previous track_id
        //Prefer suggested gate out track num if possible or lowest one possible
        QByteArray sql = "SELECT c.id,c.gate_track,c.track_side,sc.id,sc.%2_track"
                         " FROM station_gate_connections c"
                         " JOIN railway_connections sc ON sc.seg_id=?3 AND sc.%1_track=c.gate_track"
                         " WHERE c.track_id=?1 AND c.gate_id=?2"
                         " ORDER BY c.gate_track=?4 DESC,c.gate_track ASC"
                         " LIMIT 1";

        sql.replace("%1", reversed ? "out" : "in");
        sql.replace("%2", reversed ? "in" : "out");

        q.prepare(sql);
        q.bind(1, item.trackId);
        q.bind(2, seg_in_gateId);
        q.bind(3, segmentId);
        q.bind(4, suggestedOutGateTrk);
        if(q.step() != SQLITE_ROW)
        {
            //Error: gate is not connected to previous track
            //User must change previous track
            return false;
        }

        auto conn = q.getRows();
        item.toGate.gateConnId = conn.get<db_id>(0);
        item.toGate.gateId = seg_in_gateId;
        item.toGate.trackNum = conn.get<int>(1);
        item.toGate.stationTrackSide = utils::Side(conn.get<int>(2));
        item.nextSegment.segConnId = conn.get<db_id>(3);
        item.nextSegment.segmentId = segmentId;
        item.nextSegment.inTrackNum = item.toGate.trackNum;
        item.nextSegment.outTrackNum = conn.get<int>(4);
        item.nextSegment.reversed = reversed;
    }

    return true;
}

//Called for example when changing a job's shift from the ShiftGraphEditor
void StopModel::onExternalShiftChange(db_id shiftId, db_id jobId)
{
    if(jobId == mJobId)
    {
        if(shiftId == jobShiftId && shiftId != newShiftId)
            return; //This happens when notifying job was removed from previous shift, do nothing

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

void StopModel::onStationSegmentNameChanged()
{
    //Station and segment names are fetched by delegate while painting
    //We just need to repaint
    QModelIndex start = index(0, 0);
    QModelIndex end = index(stops.count(), 0);
    emit dataChanged(start, end);
}

void StopModel::reloadSettings()
{
    setAutoInsertTransits(AppSettings.getAutoInsertTransits());
    setAutoMoveUncoupleToNewLast(AppSettings.getAutoShiftLastStopCouplings());
    setAutoUncoupleAtLast(AppSettings.getAutoUncoupleAtLastStop());
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

db_id StopModel::createStop(db_id jobId, const QTime& arr, const QTime& dep, StopType type)
{
    if(type != StopType::Transit)
        type = StopType::Normal; //Fix possible invalid values

    command q_addStop(mDb, "INSERT INTO stops"
                           "(id,job_id,station_id,arrival,departure,type,description,"
                           " in_gate_conn,out_gate_conn,next_segment_conn_id)"
                           " VALUES (NULL,?,NULL,?,?,?,NULL,NULL,NULL,NULL)");
    q_addStop.bind(1, jobId);
    q_addStop.bind(2, arr);
    q_addStop.bind(3, dep);
    q_addStop.bind(4, int(type));

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    q_addStop.execute();
    db_id stopId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_addStop.reset();

    return stopId;
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

bool StopModel::updateCurrentInGate(StopItem &curStop, const StopItem::Segment &prevSeg)
{
    command cmd(mDb);

    if(!curStop.fromGate.gateConnId)
    {
        const db_id oldGateId = curStop.fromGate.gateId;
        const db_id oldTrackNum = curStop.fromGate.trackNum;

        query q(mDb, "SELECT in_gate_id,out_gate_id FROM railway_segments WHERE id=?");
        q.bind(1, prevSeg.segmentId);
        if(q.step() != SQLITE_ROW)
            return false;

        auto r = q.getRows();
        db_id segInGate = r.get<db_id>(0);
        db_id segOutGate = r.get<db_id>(1);

        q.prepare("SELECT in_track,out_track FROM railway_connections WHERE id=?");
        q.bind(1, prevSeg.segConnId);
        if(q.step() != SQLITE_ROW)
            return false;

        r = q.getRows();
        int segInTrack = r.get<int>(0);
        int segOutTrack = r.get<int>(1);

        //Segment out gate = next station in gate (unless segment is reversed)
        curStop.fromGate.gateId = prevSeg.reversed ? segInGate : segOutGate;
        curStop.fromGate.trackNum = prevSeg.reversed ? segInTrack : segOutTrack;

        //Check station
        q.prepare("SELECT station_id FROM station_gates WHERE id=?");
        q.bind(1, curStop.fromGate.gateId);
        if(q.step() != SQLITE_ROW)
            return false;

        r = q.getRows();
        db_id stationId = r.get<db_id>(0);

        if(curStop.fromGate.gateId != oldGateId || curStop.fromGate.trackNum != oldTrackNum)
        {
            //Different gate, reset track and out gate
            if(!trySelectTrackForStop(curStop))
                return false;

            if(curStop.stationId != stationId)
            {
                //Update station
                cmd.prepare("UPDATE stops SET station_id=? WHERE id=?");
                cmd.bind(1, stationId);
                cmd.bind(2, curStop.stopId);
                if(cmd.execute() != SQLITE_OK)
                    return false;

                curStop.stationId = stationId;
            }
        }
    }

    //Set gate
    cmd.prepare("UPDATE stops SET in_gate_conn=? WHERE id=?");
    cmd.bind(1, curStop.fromGate.gateConnId);
    cmd.bind(2, curStop.stopId);
    int ret = cmd.execute();
    return ret == SQLITE_OK;
}

bool StopModel::updateStopTime(StopItem &item, int row, bool propagate, const QTime& oldArr, const QTime& oldDep)
{
    //Update Arrival and Departure
    //NOTE: they must be set togheter so CHECK constraint fires at the end
    //Otherwise it would be impossible to set arrival > departure and then update departure

    //Check time values and fix them if necessary
    if(item.type == StopType::First)
        item.arrival = item.departure; //We set departure, arrival follows same value
    else if(item.type == StopType::Last || item.type == StopType::Transit)
        item.departure = item.arrival; //We set arrival, departure follows same value

    if(item.type != StopType::First && row > 0)
    {
        //Check minimum arrival
        /* Next stop must be at least one minute after
         * This is to prevent contemporary stops that will break ORDER BY arrival queries */
        const StopItem& prevStop = stops.at(row - 1);
        const QTime minArr = prevStop.departure.addSecs(60);
        if(item.arrival < minArr)
            item.arrival = minArr;
    }

    QTime minDep = item.arrival;
    if(item.type == StopType::Normal)
        minDep = minDep.addSecs(60); //At least stop for 1 minute

    if(item.departure < minDep)
        item.departure = minDep;

    //Update stops
    if(row < stops.count() - 2) //Not last stop or AddHere
    {
        const QTime minNextArr = item.departure.addSecs(60);
        const StopItem& nextStop = stops.at(row + 1);
        if(nextStop.arrival < minNextArr)
            propagate = true; //We need to shift stops after current
    }
    else
    {
        propagate = false; //We are last stop, nothing to propagate
    }

    if(propagate)
        shiftStopsBy24hoursFrom(oldArr);

    //Update Arrival and Departure in database
    command cmd(mDb);
    cmd.prepare("UPDATE stops SET arrival=?,departure=? WHERE id=?");
    cmd.bind(1, item.arrival);
    cmd.bind(2, item.departure);
    cmd.bind(3, item.stopId);

    //Mark RS to update
    query q_selectRS(mDb, "SELECT rs_id FROM coupling WHERE stop_id=?");
    q_selectRS.bind(1, item.stopId);
    for(auto rs : q_selectRS)
    {
        db_id rsId = rs.get<db_id>(0);
        rsToUpdate.insert(rsId);
    }
    q_selectRS.reset();

    if(cmd.execute() != SQLITE_OK)
        return false;

    if(propagate)
    {
        int msecOffset = oldDep.msecsTo(item.departure); //Calculate shift amount

        //Loop until Last stop (before AddHere)
        for(int i = row + 1; i < stops.count() - 1; i++)
        {
            StopItem& s = stops[i];
            s.arrival = s.arrival.addMSecs(msecOffset);
            s.departure = s.departure.addMSecs(msecOffset);

            cmd.reset();
            cmd.bind(1, s.arrival);
            cmd.bind(2, s.departure);
            cmd.bind(3, s.stopId);
            cmd.execute();

            q_selectRS.bind(1, s.stopId);
            for(auto rs : q_selectRS)
            {
                db_id rsId = rs.get<db_id>(0);
                rsToUpdate.insert(rsId);
            }
            q_selectRS.reset();

            stationsToUpdate.insert(s.stationId);
        }
    }

    return true;
}

int StopModel::calcTravelTime(db_id segmentId)
{
    DEBUG_IMPORTANT_ENTRY;

    query q(mDb, "SELECT max_speed_kmh,distance_meters FROM railway_segments WHERE id=?");
    q.bind(1, segmentId);
    if(q.step() != SQLITE_ROW)
        return 60; //Error

    auto r = q.getRows();
    const int speedKmH = r.get<int>(0);
    const int meters = r.get<int>(1);

    if(meters == 0 || speedKmH < 1.0)
        return 60; //Error

    const double secs = (meters + accelerationDistMeters)/speedKmH * 3.6;
    return qMax(60, qCeil(secs));
}

int StopModel::defaultStopTimeSec()
{
    //TODO: the prefernces should be stored also in database
    return AppSettings.getDefaultStopMins(int(category)) * 60;
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
        qWarning() << "Error while saving old stops:" << ret << mDb.error_msg() << mDb.extended_error_code();
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
