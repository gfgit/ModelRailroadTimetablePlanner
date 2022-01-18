#include "shiftgraphscene.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

static constexpr const char *sql_getStName = "SELECT name,short_name FROM stations"
                                             " WHERE id=?";

static constexpr const char *sql_countJobs = "SELECT COUNT(1) FROM jobs"
                                             " WHERE jobs.shift_id=?";

static constexpr const char *sql_getJobs = "SELECT jobs.id, jobs.category,"
                                           " MIN(s1.arrival), s1.station_id,"
                                           " MAX(s2.departure), s2.station_id"
                                           " FROM jobs"
                                           " JOIN stops s1 ON s1.job_id=jobs.id"
                                           " JOIN stops s2 ON s2.job_id=jobs.id"
                                           " WHERE jobs.shift_id=?"
                                           " GROUP BY jobs.id"
                                           " ORDER BY s1.arrival ASC";

ShiftGraphScene::ShiftGraphScene(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
    mDb(db)
{
    connect(&AppSettings, &MRTPSettings::jobColorsChanged,
            this, &ShiftGraphScene::redrawGraph);

    connect(&AppSettings, &MRTPSettings::shiftGraphOptionsChanged,
            this, &ShiftGraphScene::updateShiftGraphOptions);

    connect(Session, &MeetingSession::stationNameChanged, this, &ShiftGraphScene::onStationNameChanged);

    connect(Session, &MeetingSession::shiftNameChanged, this, &ShiftGraphScene::onShiftNameChanged);
    connect(Session, &MeetingSession::shiftRemoved, this, &ShiftGraphScene::onShiftRemoved);
    connect(Session, &MeetingSession::shiftJobsChanged, this, &ShiftGraphScene::onShiftJobsChanged);
}

bool ShiftGraphScene::loadShifts()
{
    m_shifts.clear();

    m_stationCache.clear();
    m_stationCache.squeeze();

    query q(mDb, "SELECT COUNT(1) FROM jobshifts");
    q.step();
    int count = q.getRows().get<int>(0);

    if(count == 0)
    {
        //Nothing to load
        m_shifts.squeeze();
        return true;
    }

    m_shifts.reserve(count);

    query q_getStName(mDb, sql_getStName);
    query q_countJobs(mDb, sql_countJobs);
    query q_getJobs(mDb, sql_getJobs);

    q.prepare("SELECT id,name FROM jobshifts ORDER BY name");
    for(auto shift : q)
    {
        ShiftRow obj;
        obj.shiftId = shift.get<db_id>(0);
        obj.shiftName = shift.get<QString>(1);

        loadShiftRow(obj, q_getStName, q_countJobs, q_getJobs);
        m_shifts.append(obj);
    }

    emit redrawGraph();

    return true;
}

bool ShiftGraphScene::loadShiftRow(ShiftRow &shiftObj, query &q_getStName, query &q_countJobs, sqlite3pp::query &q_getJobs)
{
    shiftObj.jobList.clear();

    q_countJobs.bind(1, shiftObj.shiftId);
    q_countJobs.step();
    int count = q_countJobs.getRows().get<int>(0);
    q_countJobs.reset();

    if(count == 0)
    {
        //Nothing to load
        return true;
    }

    shiftObj.jobList.reserve(count);

    q_getJobs.bind(1, shiftObj.shiftId);
    for(auto job : q_getJobs)
    {
        JobItem item;

        item.job.jobId = job.get<db_id>(0);
        item.job.category = JobCategory(job.get<int>(1));

        item.start = job.get<QTime>(2);
        item.fromStId = job.get<db_id>(3);

        item.end = job.get<QTime>(4);
        item.toStId = job.get<db_id>(5);

        loadStationName(item.fromStId, q_getStName);
        loadStationName(item.toStId, q_getStName);

        shiftObj.jobList.append(item);
    }
    q_getJobs.reset();

    return true;
}

void ShiftGraphScene::loadStationName(db_id stationId, sqlite3pp::query &q_getStName)
{
    if(!m_stationCache.contains(stationId))
    {
        q_getStName.bind(1, stationId);
        q_getStName.step();
        auto r = q_getStName.getRows();
        QString stName = r.get<QString>(1);

        //If 'Short Name' is empty fallback to 'Full Name'
        if(stName.isEmpty())
            stName = r.get<QString>(0);

        m_stationCache.insert(stationId, stName);
        q_getStName.reset();
    }
}

std::pair<int, int> ShiftGraphScene::lowerBound(db_id shiftId, const QString &name)
{
    //Find old shift position
    int oldIdx = -1;
    for(int i = 0; i < m_shifts.size(); i++)
    {
        if(m_shifts.at(i).shiftId == shiftId)
        {
            oldIdx = i;
            break;
        }
    }

    //Find new position to insert
    int firstIdx = 0;
    int len = m_shifts.size();

    while (len > 0)
    {
        int half = len >> 1;
        int middleIdx = firstIdx + half;

        const ShiftRow& obj = m_shifts.at(middleIdx);
        if(obj.shiftId != shiftId && obj.shiftName < name)
        {
            firstIdx = middleIdx + 1;
            len = len - half - 1;
        }
        else
        {
            len = half;
        }
    }

    return {oldIdx, firstIdx};
}

void ShiftGraphScene::updateShiftGraphOptions()
{
    hourOffset = AppSettings.getShiftHourOffset();
    horizOffset = AppSettings.getShiftHorizOffset();
    vertOffset = AppSettings.getShiftVertOffset();

    jobOffset = AppSettings.getShiftJobOffset();
    jobBoxOffset = AppSettings.getShiftJobBoxOffset();
    stationNameOffset = AppSettings.getShiftStationOffset();
    hideSameStations = AppSettings.getShiftHideSameStations();

    emit redrawGraph();
}

void ShiftGraphScene::onShiftNameChanged(db_id shiftId)
{
    query q(mDb, "SELECT name FROM jobshifts WHERE id=?");
    q.bind(1, shiftId);
    if(q.step() != SQLITE_ROW)
        return;

    QString newName = q.getRows().get<QString>(0);

    auto pos = lowerBound(shiftId, newName);

    if(pos.first == -1)
    {
        //This shift is new, add it
        ShiftRow obj;
        obj.shiftId = shiftId;
        obj.shiftName = newName;

        //Load jobs
        query q_getStName(mDb, sql_getStName);
        query q_countJobs(mDb, sql_countJobs);
        query q_getJobs(mDb, sql_getJobs);
        loadShiftRow(obj, q_getStName, q_countJobs, q_getJobs);

        m_shifts.insert(pos.second, obj);
    }
    else
    {
        //Move shift to keep alphabetical order
        m_shifts.move(pos.first, pos.second);
    }
}

void ShiftGraphScene::onShiftRemoved(db_id shiftId)
{
    for(auto it = m_shifts.begin(); it != m_shifts.end(); it++)
    {
        if(it->shiftId == shiftId)
        {
            m_shifts.erase(it);
            break;
        }
    }

    emit redrawGraph();
}

void ShiftGraphScene::onShiftJobsChanged(db_id shiftId)
{
    //Reload single shift
    query q_getStName(mDb, sql_getStName);
    query q_countJobs(mDb, sql_countJobs);
    query q_getJobs(mDb, sql_getJobs);

    for(ShiftRow& shift : m_shifts)
    {
        if(shift.shiftId == shiftId)
        {
            loadShiftRow(shift, q_getStName, q_countJobs, q_getJobs);
            break;
        }
    }

    emit redrawGraph();
}

void ShiftGraphScene::onStationNameChanged(db_id stationId)
{
    if(m_stationCache.contains(stationId))
    {
        //Reload name
        m_stationCache.remove(stationId);

        query q_getStName(mDb, sql_getStName);
        loadStationName(stationId, q_getStName);

        emit redrawGraph();
    }
}
