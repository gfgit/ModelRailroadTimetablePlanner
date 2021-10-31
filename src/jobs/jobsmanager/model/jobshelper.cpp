#include "jobshelper.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QDebug>

bool JobsHelper::createNewJob(sqlite3pp::database &db, db_id &outJobId)
{
    sqlite3pp::command q_newJob(db, "INSERT INTO jobs(id,category,shift_id) VALUES(NULL,0,NULL)");

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

    db.execute("BEGIN TRANSACTION");

    q.prepare("DELETE FROM stops WHERE job_id=?");

    q.bind(1, jobId);
    int ret = q.step();
    q.reset();

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

    return true;
}

bool JobsHelper::removeAllJobs(sqlite3pp::database &db)
{
    //Old
    sqlite3pp::command cmd(db, "DELETE FROM old_coupling");
    cmd.execute();

    cmd.prepare("DELETE FROM old_stops");
    cmd.execute();

    cmd.prepare("DELETE FROM old_jobsegments");
    cmd.execute();

    //Current
    cmd.prepare("DELETE FROM coupling");
    cmd.execute();

    cmd.prepare("DELETE FROM stops");
    cmd.execute();

    cmd.prepare("DELETE FROM jobsegments");
    cmd.execute();

    //Delete jobs
    cmd.prepare("DELETE FROM jobs");
    cmd.execute();

    emit Session->jobRemoved(0);

    return true;
}
