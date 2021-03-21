#include "jobstorage.h"

#include "traingraphics.h"

#include <QSet>

#include <QDebug>

#include "app/session.h"

#include <QGraphicsPathItem>

#include "lines/linestorage.h"

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
    connect(&AppSettings, &TrainTimetableSettings::jobColorsChanged, this, &JobStorage::updateJobColors);
    connect(Session, &MeetingSession::jobChanged, this, &JobStorage::updateJob);
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

void JobStorage::drawJobs()
{
    for(auto it = impl->m_data.begin(); it != impl->m_data.end(); )
    {
        TrainGraphics &tg = it.value();
        tg.recalcPath();
        if(tg.segments.isEmpty())
        {
            //No line of this job is currently loaded so discard job
            it->clear();
            it = impl->m_data.erase(it);
        }else{
            it++;
        }
    }
}

void JobStorage::updateJobColors()
{
    for(TrainGraphics &tg : impl->m_data)
    {
        tg.updateColor();
    }
}

void JobStorage::updateJob(db_id newId, db_id oldId)
{
    auto it = impl->m_data.find(oldId);
    if(it == impl->m_data.end())
        return;

    if(newId != oldId)
    {
        TrainGraphics tg = it.value(); //Deep copy
        impl->m_data.erase(it);
        it = impl->m_data.insert(newId, tg);
    }

    query q_getCat(mDb, "SELECT category FROM jobs WHERE id=?");
    q_getCat.bind(1, newId);
    q_getCat.step();
    JobCategory cat = JobCategory(q_getCat.getRows().get<int>(0));
    it.value().setId(newId);
    it.value().setCategory(cat);
}

void JobStorage::updateJobPath(db_id jobId)
{
    auto it = impl->m_data.find(jobId);
    if(it == impl->m_data.end())
    {
        //Give a chance to reload, look for a loaded line
        query q(mDb, "SELECT category FROM jobs WHERE id=?");
        q.bind(1, jobId);
        int ret = q.step();
        JobCategory cat = JobCategory(q.getRows().get<int>(0));
        if(ret != SQLITE_ROW)
            return;

        TrainGraphics tg(jobId, cat);
        it = impl->m_data.insert(jobId, tg);
    }

    TrainGraphics& tg = it.value();
    tg.recalcPath();

    if(tg.segments.isEmpty())
    {
        //No line of this job is currently loaded so discard job
        tg.clear();
        impl->m_data.erase(it);
    }
}

void JobStorage::drawJobs(db_id lineId)
{
    query q_getSegments(mDb, "SELECT id, jobId FROM jobsegments WHERE lineId=?");

    q_getSegments.bind(1, lineId);
    for(auto seg : q_getSegments)
    {
        db_id segId = seg.get<db_id>(0);
        db_id jobId = seg.get<db_id>(1);

        auto tg = impl->m_data.find(jobId);
        if(tg == impl->m_data.end() || !tg.value().segments.contains(segId))
            continue;

        tg.value().drawSegment(segId, lineId);
    }
    q_getSegments.reset();
}

void JobStorage::loadLine(db_id lineId)
{
    query q_getSegments(mDb, "SELECT id, jobId FROM jobsegments WHERE lineId=? ORDER BY jobId");
    query q_getJobCat(mDb, "SELECT category FROM jobs WHERE id=?");

    q_getSegments.bind(1, lineId);
    for(auto seg : q_getSegments)
    {
        db_id segId = seg.get<db_id>(0);
        db_id jobId = seg.get<db_id>(1);

        auto it = impl->m_data.find(jobId);
        if(it == impl->m_data.end())
        {
            q_getJobCat.bind(1, jobId);
            int ret = q_getJobCat.step();
            JobCategory cat = JobCategory(q_getJobCat.getRows().get<int>(0));
            q_getJobCat.reset();
            if(ret != SQLITE_ROW)
                continue;

            TrainGraphics tg(jobId, cat);
            it = impl->m_data.insert(jobId, tg);
        }

        TrainGraphics &ref = it.value();
        ref.createSegment(segId);
    }
    q_getSegments.reset();
}

void JobStorage::unloadLine(db_id lineId)
{
    if(impl->m_data.isEmpty() || !mDb.db())
        return;

    query q_getSegments(mDb, "SELECT id, jobId FROM jobsegments WHERE lineId=? ORDER BY jobId");

    q_getSegments.bind(1, lineId);
    for(auto seg : q_getSegments)
    {
        db_id segId = seg.get<db_id>(0);
        db_id jobId = seg.get<db_id>(1);

        auto it = impl->m_data.find(jobId);
        if(it == impl->m_data.end())
            continue;

        TrainGraphics &tg = it.value();
        auto segGraph = tg.segments.find(segId);
        if(segGraph != tg.segments.end())
        {
            segGraph->cleanup();
            tg.segments.erase(segGraph);
        }

        //Because we ORDER BY we work on a single job for a few cycles
        //Last cycle may have empty job (removed all segments) check every cycle
        if(tg.segments.isEmpty())
        {
            //Unload job
            tg.clear();
            impl->m_data.erase(it);
        }
    }
    q_getSegments.reset();
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
