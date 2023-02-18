#include "jobmatchmodel.h"

#include "utils/jobcategorystrings.h"

#include "app/session.h"

JobMatchModel::JobMatchModel(database &db, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    q_getMatches(mDb),
    m_exceptJobId(0),
    m_stopStationId(0),
    m_maxStopArrival(),
    m_defaultId(JobId)
{
    m_font.setPointSize(13);
    setHasEmptyRow(false);
}

int JobMatchModel::columnCount(const QModelIndex &p) const
{
    if(p.isValid())
        return 0;

    if(!m_stopStationId)
        return NCols - 1; //Hide stop arrival if no station filter is set
    return NCols;
}

QVariant JobMatchModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= size)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case JobCategoryCol:
        {
            if(isEllipsesRow(idx.row()))
            {
                break;
            }
            return JobCategoryName::shortName(items[idx.row()].stop.category);
        }
        case JobNumber:
        {
            if(isEllipsesRow(idx.row()))
            {
                return ellipsesString;
            }
            return items[idx.row()].stop.jobId;
        }
        case StopArrivalCol:
        {
            if(!m_stopStationId)
                break; //Do not show stop arrival if not filtering by station

            return items[idx.row()].stopArrival;
        }
        }
        break;
    }
    case Qt::ForegroundRole:
    {
        if(isEllipsesRow(idx.row()))
        {
            break;
        }
        switch (idx.column())
        {
        case JobCategoryCol:
        {
            return Session->colorForCat(items[idx.row()].stop.category);
        }
        case JobNumber:
        case StopArrivalCol:
            return QColor(Qt::black);
        }
        break;
    }
    case Qt::FontRole:
    {
        switch (idx.column())
        {
        case JobCategoryCol:
        {
            QFont f = m_font;
            f.setWeight(QFont::Bold);
            return f;
        }
        case JobNumber:
        case StopArrivalCol:
            return m_font;
        }
    }
    }

    return QVariant();
}

void JobMatchModel::autoSuggest(const QString &text)
{
    mQuery.clear();
    if(!text.isEmpty())
    {
        mQuery.reserve(text.size() + 2);
        mQuery.append('%');
        mQuery.append(text.toUtf8());
        mQuery.append('%');
    }

    refreshData();
}

void JobMatchModel::refreshData()
{
    if(!mDb.db())
        return;

    beginResetModel();

    char emptyQuery = '%';

    if(mQuery.isEmpty())
        sqlite3_bind_text(q_getMatches.stmt(), 1, &emptyQuery, 1, SQLITE_STATIC);
    else
        sqlite3_bind_text(q_getMatches.stmt(), 1, mQuery, mQuery.size(), SQLITE_STATIC);

    if(m_exceptJobId)
        q_getMatches.bind(2, m_exceptJobId);

    if(m_stopStationId)
    {
        q_getMatches.bind(3, m_stopStationId);
        if(!m_maxStopArrival.isNull())
            q_getMatches.bind(4, m_maxStopArrival);
    }

    auto end = q_getMatches.end();
    auto it = q_getMatches.begin();
    int i = 0;
    for(; i < MaxMatchItems && it != end; i++)
    {
        items[i].stop.jobId = (*it).get<db_id>(0);
        items[i].stop.category = JobCategory((*it).get<int>(1));
        if(m_stopStationId)
        {
            items[i].stop.stopId = (*it).get<db_id>(2);
            items[i].stopArrival = (*it).get<QTime>(3);
        }

        ++it;
    }

    size = i;
    if(hasEmptyRow)
        size++; //Items + Empty, add 1 row

    if(it != end)
    {
        //There would be still rows, show Ellipses
        size++; //Items + Empty + Ellispses
    }

    q_getMatches.reset();
    endResetModel();

    emit resultsReady(false);
}

QString JobMatchModel::getName(db_id jobId) const
{
    if(!mDb.db())
        return QString();

    query q(mDb, "SELECT category FROM jobs WHERE id=?");
    q.bind(1, jobId);
    if(q.step() != SQLITE_ROW)
        return QString();

    JobCategory jobCat = JobCategory(q.getRows().get<int>(0));
    return JobCategoryName::jobName(jobId, jobCat);
}

db_id JobMatchModel::getIdAtRow(int row) const
{
    if(m_defaultId == StopId)
        return items[row].stop.stopId;
    return items[row].stop.jobId;
}

QString JobMatchModel::getNameAtRow(int row) const
{
    return JobCategoryName::jobName(items[row].stop.jobId,
                                    items[row].stop.category);
}

void JobMatchModel::setFilter(db_id exceptJobId, db_id stopsInStationId, const QTime &maxStopArr)
{
    m_exceptJobId = exceptJobId;
    m_stopStationId = stopsInStationId;
    m_maxStopArrival = maxStopArr;

    QByteArray sql;
    if(m_stopStationId)
    {
        //Filter by stopping station
        sql = "SELECT stops.job_id, jobs.category, stops.id, stops.arrival"
              " FROM stops JOIN jobs ON jobs.id=stops.job_id"
              " WHERE stops.station_id=?3 AND";

        if(!m_maxStopArrival.isNull())
            sql += " stops.arrival < ?4 AND";
    }
    else
    {
        sql = "SELECT jobs.id, jobs.category FROM jobs WHERE";
    }

    if(exceptJobId)
    {
        //Filter out unwanted job ID
        sql += " jobs.id<>?2 AND";
    }

    //Filter by name (job ID number)
    //FIXME: filter also by category with regexp
    sql += " jobs.id LIKE ?1";

    sql += " ORDER BY ";
    if(m_stopStationId)
    {
        sql += "stops.arrival, ";
    }
    sql += "jobs.category, jobs.id";

    sql += " LIMIT " QT_STRINGIFY(MaxMatchItems + 1);

    q_getMatches.prepare(sql);
}

void JobMatchModel::setDefaultId(DefaultId defaultId)
{
    m_defaultId = defaultId;
}
