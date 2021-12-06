#include "jobshelper.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QDebug>

bool JobsHelper::createNewJob(sqlite3pp::database &db, db_id &outJobId, JobCategory cat)
{
    sqlite3pp::command q_newJob(db, "INSERT INTO jobs(id,category,shift_id) VALUES(NULL,?,NULL)");
    q_newJob.bind(1, int(cat));

    sqlite3_mutex *mutex = sqlite3_db_mutex(db.db());
    sqlite3_mutex_enter(mutex);
    int rc = q_newJob.execute();
    db_id jobId = db.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_newJob.reset();

    if(rc != SQLITE_OK)
    {
        qWarning() << rc << db.error_msg();
        outJobId = 0;
        return false;
    }

    outJobId = jobId;

    emit Session->jobAdded(jobId);

    return true;
}

bool JobsHelper::removeJob(sqlite3pp::database &db, db_id jobId)
{
    sqlite3pp::query q(db, "SELECT shift_id FROM jobs WHERE id=?");
    q.bind(1, jobId);
    if(q.step() != SQLITE_ROW)
    {
        return false; //Job doesn't exist
    }
    db_id shiftId = q.getRows().get<db_id>(0);
    q.reset();

    if(shiftId != 0)
    {
        //Remove job from shift
        emit Session->shiftJobsChanged(shiftId, jobId);
    }

    //Get stations in which job stopped or transited
    QSet<db_id> stationsToUpdate;
    q.prepare("SELECT station_id FROM stops WHERE job_id=?"
              " UNION "
              "SELECT station_id FROM old_stops WHERE job_id=?");
    q.bind(1, jobId);
    for(auto st : q)
    {
        stationsToUpdate.insert(st.get<db_id>(0));
    }

    //Get Rollingstock used by job
    QSet<db_id> rsToUpdate;
    q.prepare("SELECT coupling.rs_id FROM stops JOIN coupling ON coupling.stop_id=stops.id"
              " WHERE stops.job_id=?"
              " UNION "
              "SELECT old_coupling.rs_id FROM old_stops JOIN old_coupling ON old_coupling.stop_id=old_stops.id"
              " WHERE old_stops.job_id=?");
    q.bind(1, jobId);
    for(auto rs : q)
    {
        rsToUpdate.insert(rs.get<db_id>(0));
    }

    //Remove job
    db.execute("BEGIN TRANSACTION");

    q.prepare("DELETE FROM stops WHERE job_id=?");

    q.bind(1, jobId);
    int ret = q.step();
    q.reset();

    if(ret == SQLITE_OK || ret == SQLITE_DONE)
    {
        //Remove possible left over from editing
        q.prepare("DELETE FROM old_stops WHERE job_id=?");
        q.bind(1, jobId);
        ret = q.step();
        q.reset();
    }

    if(ret == SQLITE_OK || ret == SQLITE_DONE)
    {
        q.prepare("DELETE FROM jobs WHERE id=?");
        q.bind(1, jobId);
        ret = q.step();
        q.reset();
    }

    if(ret == SQLITE_OK || ret == SQLITE_DONE)
    {
        db.execute("COMMIT");
    }
    else
    {
        qDebug() << "Error while removing Job:" << jobId << ret << db.error_msg() << db.extended_error_code();
        db.execute("ROLLBACK");
        return false;
    }

    emit Session->jobRemoved(jobId);

    //Refresh graphs and station views
    emit Session->stationPlanChanged(stationsToUpdate);

    //Refresh Rollingstock views
    emit Session->rollingStockPlanChanged(rsToUpdate);

    return true;
}

bool JobsHelper::removeAllJobs(sqlite3pp::database &db)
{
    //Old
    sqlite3pp::command cmd(db, "DELETE FROM old_coupling");
    cmd.execute();

    cmd.prepare("DELETE FROM old_stops");
    cmd.execute();

    //Current
    cmd.prepare("DELETE FROM coupling");
    cmd.execute();

    cmd.prepare("DELETE FROM stops");
    cmd.execute();

    //Delete jobs
    cmd.prepare("DELETE FROM jobs");
    cmd.execute();

    emit Session->jobRemoved(0);

    return true;
}

bool JobsHelper::copyStops(sqlite3pp::database &db, db_id fromJobId, db_id toJobId, int secsOffset, bool copyRsOps)
{
    query q_getStops(db, "SELECT id,station_id,arrival,departure,type,"
                         "description,in_gate_conn,out_gate_conn,next_segment_conn_id"
                         " FROM stops WHERE job_id=?");
    query q_getRsOp(db, "SELECT rs_id,operation FROM coupling WHERE stop_id=?");

    command q_setStop(db, "INSERT INTO stops(id,job_id,station_id,arrival,departure,type,"
                          "description,in_gate_conn,out_gate_conn,next_segment_conn_id)"
                          "VALUES(NULL,?,?,?,?,?,?,?,?,?)");
    command q_setRsOp(db, "INSERT INTO coupling(id,stop_id,rs_id,operation) VALUES(NULL,?,?,?)");

    QSet<db_id> stationsToUpdate;
    QSet<db_id> rsToUpdate;

    q_getStops.bind(1, fromJobId);
    for(auto stop : q_getStops)
    {
        db_id origStopId = stop.get<db_id>(0);
        db_id stationId = stop.get<db_id>(1);
        QTime arrival = stop.get<QTime>(2);
        QTime departure = stop.get<QTime>(3);
        int type = stop.get<int>(4);

        //Avoid memory copy
        const unsigned char *description = sqlite3_column_text(q_getStops.stmt(), 5);
        const int descrLen = sqlite3_column_bytes(q_getStops.stmt(), 5);

        db_id in_gate_conn = stop.get<db_id>(6);
        db_id out_gate_conn = stop.get<db_id>(7);
        db_id next_seg_conn = stop.get<db_id>(8);

        //Apply time shift
        arrival = arrival.addSecs(secsOffset);
        departure = departure.addSecs(secsOffset);

        q_setStop.bind(1, toJobId);
        q_setStop.bindOrNull(2, stationId);
        q_setStop.bind(3, arrival);
        q_setStop.bind(4, departure);
        q_setStop.bind(5, type);
        //Pass SQLITE_STATIC because description is valid until next loop cycle, so avoid copy
        sqlite3_bind_text(q_setStop.stmt(), 6, reinterpret_cast<const char *>(description), descrLen, SQLITE_STATIC);
        q_setStop.bindOrNull(7, in_gate_conn);
        q_setStop.bindOrNull(8, out_gate_conn);
        q_setStop.bindOrNull(9, next_seg_conn);
        if(q_setStop.execute() != SQLITE_OK)
        {
            qWarning() << "JobsHelper::copyStops() error setting stop" << origStopId << "To:" << toJobId << secsOffset
                       << db.error_msg();
            continue; //Skip stop
        }
        db_id newStopId = db.last_insert_rowid();
        q_setStop.reset();

        if(copyRsOps)
        {
            q_getRsOp.bind(1, origStopId);
            for(auto rs : q_getRsOp)
            {
                db_id rsId = rs.get<db_id>(0);
                int op = rs.get<int>(1);

                q_setRsOp.bind(1, newStopId);
                q_setRsOp.bind(2, rsId);
                q_setRsOp.bind(3, op);
                q_setRsOp.execute();
                q_setRsOp.reset();

                //Store rollingstock ID to update it later
                rsToUpdate.insert(rsId);
            }
            q_getRsOp.reset();
        }

        //Store station to update it later
        stationsToUpdate.insert(stationId);
    }

    //Refresh graphs and station views
    emit Session->stationPlanChanged(stationsToUpdate);

    //Refresh Rollingstock views
    emit Session->rollingStockPlanChanged(rsToUpdate);

    return true;
}

JobStopDirectionHelper::JobStopDirectionHelper(sqlite3pp::database &db) :
    mDb(db),
    m_query(new sqlite3pp::query(mDb))
{
    int ret = m_query->prepare("SELECT g1.side,g2.side"
                               " FROM stops"
                               " LEFT JOIN station_gate_connections c1 ON c1.id=stops.in_gate_conn"
                               " LEFT JOIN station_gate_connections c2 ON c2.id=stops.out_gate_conn"
                               " LEFT JOIN station_gates g1 ON g1.id=c1.gate_id"
                               " LEFT JOIN station_gates g2 ON g2.id=c2.gate_id"
                               " WHERE stops.id=?");
    if(ret != SQLITE_OK)
        qWarning() << "JobStopDirectionHelper cannot prepare query";
}

JobStopDirectionHelper::~JobStopDirectionHelper()
{
    delete m_query;
    m_query = nullptr;
}

utils::Side JobStopDirectionHelper::getStopOutSide(db_id stopId)
{
    m_query->bind(1, stopId);
    if(m_query->step() != SQLITE_ROW)
    {
        //Stop doesn't exist
        return utils::Side::NSides;
    }

    utils::Side in_side = utils::Side::NSides;
    utils::Side out_side = utils::Side::NSides;

    auto r = m_query->getRows();
    if(r.column_type(0) != SQLITE_NULL)
        in_side = utils::Side(r.get<int>(0));
    if(r.column_type(1) != SQLITE_NULL)
        out_side = utils::Side(r.get<int>(1));

    //Prefer out side
    if(out_side != utils::Side::NSides)
        return out_side;

    //We only have in side, invert it
    if(in_side == utils::Side::NSides)
        return in_side;

    return in_side == utils::Side::East ? utils::Side::West : utils::Side::East;
}
