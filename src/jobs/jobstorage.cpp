#include "jobstorage.h"

#include "traingraphics.h"

#include <QSet>

#include <QDebug>

#include "app/session.h"

#include <QGraphicsPathItem>

class JobStoragePrivate
{
public:
    typedef QHash<db_id, TrainGraphics> Data;
    Data m_data;
};

JobStorage::JobStorage(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
    mDb(db)
{
    impl = new JobStoragePrivate;
}

JobStorage::~JobStorage()
{
    clear();
    delete impl;
    impl = nullptr;
}

void JobStorage::clear()
{
    for(TrainGraphics &tg : impl->m_data)
        tg.clear();
    impl->m_data.clear();
    impl->m_data.squeeze();
}

bool JobStorage::addJob(db_id *outJobId)
{
    command q_newJob(mDb, "INSERT INTO jobs(id,category,firstStop,lastStop, shiftId) VALUES(NULL,0,NULL,NULL,NULL)");

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int rc = q_newJob.execute();
    db_id jobId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_newJob.reset();

    if(rc != SQLITE_OK)
    {
        qWarning() << rc << mDb.error_msg();
        if(outJobId)
            *outJobId = 0;
        return false;
    }

    if(outJobId)
        *outJobId = jobId;

    emit jobAdded(jobId);

    return true;
}

bool JobStorage::removeJob(db_id jobId)
{
    query q(mDb, "SELECT shiftId FROM jobs WHERE id=?");
    q.bind(1, jobId);
    if(q.step() != SQLITE_ROW)
    {
        return false; //Job doesn't exist
    }
    db_id shiftId = q.getRows().get<db_id>(0);
    q.reset();

    //TODO: inform RS and stations
    emit aboutToRemoveJob(jobId);

    if(shiftId != 0)
    {
        //Remove job from shift
        emit Session->shiftJobsChanged(shiftId, jobId);
    }

    mDb.execute("BEGIN TRANSACTION");

    q.prepare("DELETE FROM stops WHERE jobId=?");

    q.bind(1, jobId);
    int ret = q.step();
    q.reset();

    if(ret == SQLITE_OK || ret == SQLITE_DONE)
    {
        q.prepare("DELETE FROM jobsegments WHERE jobId=?");
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
        mDb.execute("COMMIT");
    }
    else
    {
        qDebug() << "Error while removing Job:" << jobId << ret << mDb.error_msg() << mDb.extended_error_code();
        mDb.execute("ROLLBACK");
        return false;
    }

    auto it = impl->m_data.find(jobId);
    if(it != impl->m_data.end())
    {
        TrainGraphics &tg = it.value();
        tg.clear();
        impl->m_data.erase(it);
    }

    emit jobRemoved(jobId);

    return true;
}

void JobStorage::removeAllJobs()
{
    //Old
    command cmd(mDb, "DELETE FROM old_coupling");
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

    clear();

    emit jobRemoved(0);
}

void JobStorage::updateFirstLast(db_id jobId)
{
    command q(mDb, "UPDATE jobs"
                   " SET firstStop=(SELECT id"
                   " FROM(SELECT id, MIN(departure)"
                   " FROM stops WHERE jobId=?1)"
                   ") WHERE id=?1");

    q.bind(1, jobId);
    q.execute();
    q.reset();

    q.prepare("UPDATE jobs"
              " SET lastStop=(SELECT id"
              " FROM(SELECT id, MAX(departure)"
              " FROM stops WHERE jobId=?1)"
              ") WHERE id=?1");



    q.bind(1, jobId);
    q.execute();
    q.reset();
}

bool JobStorage::selectSegment(db_id jobId, db_id segId, bool select, bool ensureVisible)
{
    auto it = impl->m_data.find(jobId);
    if(it == impl->m_data.end())
        return false;

    TrainGraphics &tg = it.value();
    auto seg = tg.segments.constFind(segId);
    if(seg == tg.segments.constEnd())
        return false;

    seg.value().item->setSelected(select);
    if(ensureVisible)
        seg.value().item->ensureVisible();

    return true;
}
