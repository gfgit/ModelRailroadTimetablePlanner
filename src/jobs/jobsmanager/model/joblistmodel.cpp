/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "joblistmodel.h"

#include "utils/delegates/sql/pageditemmodelhelper_impl.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/jobcategorystrings.h"

#include <QDebug>

JobListModel::JobListModel(sqlite3pp::database &db, QObject *parent) :
    BaseClass(500, db, parent)
{
    sortColumn = IdCol;

    connect(Session, &MeetingSession::shiftNameChanged, this, &IPagedItemModel::clearCache_slot);
    connect(Session, &MeetingSession::shiftJobsChanged, this, &IPagedItemModel::clearCache_slot);
    connect(Session, &MeetingSession::stationNameChanged, this, &IPagedItemModel::clearCache_slot);
    connect(Session, &MeetingSession::jobChanged, this, &IPagedItemModel::clearCache_slot);

    connect(Session, &MeetingSession::jobAdded, this, &JobListModel::onJobAddedOrRemoved);
    connect(Session, &MeetingSession::jobRemoved, this, &JobListModel::onJobAddedOrRemoved);
}

QVariant JobListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case IdCol:
                return tr("Number");
            case Category:
                return tr("Category");
            case ShiftCol:
                return tr("Shift");
            case OriginSt:
                return tr("Origin");
            case OriginTime:
                return tr("Departure");
            case DestinationSt:
                return tr("Destination");
            case DestinationTime:
                return tr("Arrival");
            default:
                break;
            }
        }
        else
        {
            return section + curPage * ItemsPerPage + 1;
        }
    }
    return IPagedItemModel::headerData(section, orientation, role);
}

QVariant JobListModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        // Fetch above or below current cache
        const_cast<JobListModel *>(this)->fetchRow(row);

        // Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const JobItem &item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case IdCol:
            return item.jobId;
        case Category:
            return JobCategoryName::shortName(item.category);
        case ShiftCol:
            return item.shiftName;
        case OriginSt:
            return item.origStName;
        case OriginTime:
            return item.originTime;
        case DestinationSt:
            return item.destStName;
        case DestinationTime:
            return item.destTime;
        default:
            break;
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        if (idx.column() == IdCol)
        {
            return QVariant::fromValue(Qt::AlignVCenter | Qt::AlignRight);
        }
        break;
    }
    }

    return QVariant();
}

void JobListModel::setSortingColumn(int col)
{
    if (sortColumn == col || col == OriginSt || col == DestinationSt || col >= NCols)
        return;

    clearCache();
    sortColumn        = col;

    QModelIndex first = index(0, 0);
    QModelIndex last  = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

std::pair<QString, IPagedItemModel::FilterFlags> JobListModel::getFilterAtCol(int col)
{
    switch (col)
    {
    case IdCol:
        return {m_jobIdFilter, FilterFlag::BasicFiltering};
    case ShiftCol:
        return {m_shiftFilter, FilterFlags(FilterFlag::BasicFiltering | FilterFlag::ExplicitNULL)};
    }

    return {QString(), FilterFlag::NoFiltering};
}

bool JobListModel::setFilterAtCol(int col, const QString &str)
{
    const bool isNull = str.startsWith(nullFilterStr, Qt::CaseInsensitive);

    switch (col)
    {
    case IdCol:
    {
        if (isNull)
            return false; // Cannot have NULL Job ID
        m_jobIdFilter = str;
        break;
    }
    case ShiftCol:
    {
        m_shiftFilter = str;
        break;
    }
    default:
        return false;
    }

    emit filterChanged();
    return true;
}

void JobListModel::onJobAddedOrRemoved()
{
    refreshData(); // Recalc row count
}

qint64 JobListModel::recalcTotalItemCount()
{
    query q(mDb);
    buildQuery(q, 0, 0, false);

    q.step();
    const qint64 count = q.getRows().get<int>(0);
    return count;
}

void JobListModel::buildQuery(sqlite3pp::query &q, int sortCol, int offset, bool fullData)
{
    QByteArray sql;
    if (fullData)
    {
        sql = "SELECT jobs.id, jobs.category, jobs.shift_id, jobshifts.name,"
              "MIN(s1.departure), s1.station_id, MAX(s2.arrival), s2.station_id"
              " FROM jobs"
              " LEFT JOIN stops s1 ON s1.job_id=jobs.id"
              " LEFT JOIN stops s2 ON s2.job_id=jobs.id";
    }
    else
    {
        sql = "SELECT COUNT(1) FROM jobs";
    }

    // If counting but filtering by shift name (not null) we need to JOIN jobshifts
    bool shiftFilterIsNull = m_shiftFilter.startsWith(nullFilterStr, Qt::CaseInsensitive);
    if (fullData || (!shiftFilterIsNull && !m_shiftFilter.isEmpty()))
        sql += " LEFT JOIN jobshifts ON jobshifts.id=jobs.shift_id";

    bool whereClauseAdded = false;

    if (!m_jobIdFilter.isEmpty())
    {
        sql.append(" WHERE jobs.id LIKE ?3");
        whereClauseAdded = true;
    }

    if (!m_shiftFilter.isEmpty())
    {
        if (whereClauseAdded)
            sql.append(" AND ");
        else
            sql.append(" WHERE ");

        if (shiftFilterIsNull)
            sql.append("jobs.shift_id IS NULL");
        else
            sql.append("jobshifts.name LIKE ?4");
    }

    if (fullData)
    {
        // Group by Job
        sql.append(" GROUP BY jobs.id");

        // Apply sorting
        const char *sortColExpr = nullptr;
        switch (sortCol)
        {
        case IdCol:
        {
            sortColExpr = "jobs.id"; // Order by 1 column, no where clause
            break;
        }
        case Category:
        {
            sortColExpr = "jobs.category,jobs.id";
            break;
        }
        case ShiftCol:
        {
            sortColExpr = "jobshifts.name,s1.departure,jobs.id";
            break;
        }
        case OriginTime:
        {
            sortColExpr = "s1.departure,jobs.id";
            break;
        }
        case DestinationTime:
        {
            sortColExpr = "s2.arrival,jobs.id";
            break;
        }
        }

        sql += " ORDER BY ";
        sql += sortColExpr;

        sql += " LIMIT ?1";
        if (offset)
            sql += " OFFSET ?2";
    }

    q.prepare(sql);

    if (fullData)
    {
        // Apply offset and batch size
        q.bind(1, BatchSize);
        if (offset)
            q.bind(2, offset);
    }

    // Apply filters
    QByteArray jobFilter;
    if (!m_jobIdFilter.isEmpty())
    {
        jobFilter.reserve(m_jobIdFilter.size() + 2);
        jobFilter.append('%');
        jobFilter.append(m_jobIdFilter.toUtf8());
        jobFilter.append('%');
        sqlite3_bind_text(q.stmt(), 3, jobFilter, jobFilter.size(), SQLITE_STATIC);
    }

    QByteArray shiftFilter;
    if (!m_shiftFilter.isEmpty() && !shiftFilterIsNull)
    {
        shiftFilter.reserve(m_shiftFilter.size() + 2);
        shiftFilter.append('%');
        shiftFilter.append(m_shiftFilter.toUtf8());
        shiftFilter.append('%');
        sqlite3_bind_text(q.stmt(), 4, shiftFilter, shiftFilter.size(), SQLITE_STATIC);
    }
}

void JobListModel::internalFetch(int first, int sortCol, int /*valRow*/, const QVariant & /*val*/)
{
    query q(mDb);

    query q_stationName(mDb, "SELECT name FROM stations WHERE id=?");

    int offset = first + curPage * ItemsPerPage;

    qDebug() << "Fetching:" << first << "Offset:" << offset;
    buildQuery(q, sortCol, offset, true);

    QList<JobItem> vec(BatchSize);

    // QString are implicitly shared, use QHash to temporary store them instead
    // of creating new ones for each JobItem
    QHash<db_id, QString> shiftHash;
    QHash<db_id, QString> stationHash;

    auto it             = q.begin();
    const auto end      = q.end();

    int i               = 0;
    const int increment = 1;

    for (; it != end; ++it)
    {
        auto r        = *it;
        JobItem &item = vec[i];
        item.jobId    = r.get<db_id>(0);
        item.category = JobCategory(r.get<int>(1));
        item.shiftId  = r.get<db_id>(2);

        if (item.shiftId)
        {
            auto shift = shiftHash.constFind(item.shiftId);
            if (shift == shiftHash.constEnd())
            {
                shift = shiftHash.insert(item.shiftId, r.get<QString>(3));
            }
            item.shiftName = shift.value();
        }

        item.originTime = r.get<QTime>(4);
        item.originStId = r.get<db_id>(5);
        item.destTime   = r.get<QTime>(6);
        item.destStId   = r.get<db_id>(7);

        if (item.originStId)
        {
            auto st = stationHash.constFind(item.originStId);
            if (st == stationHash.constEnd())
            {
                q_stationName.bind(1, item.originStId);
                q_stationName.step();
                st = stationHash.insert(item.originStId, q_stationName.getRows().get<QString>(0));
                q_stationName.reset();
            }
            item.origStName = st.value();
        }

        if (item.destStId)
        {
            auto st = stationHash.constFind(item.destStId);
            if (st == stationHash.constEnd())
            {
                q_stationName.bind(1, item.destStId);
                q_stationName.step();
                st = stationHash.insert(item.destStId, q_stationName.getRows().get<QString>(0));
                q_stationName.reset();
            }
            item.destStName = st.value();
        }

        i += increment;
    }

    if (i < BatchSize)
        vec.remove(i, BatchSize - i);

    postResult(vec, first);
}
