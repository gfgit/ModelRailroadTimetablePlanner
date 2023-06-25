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

#include "stationfreersmodel.h"

#include "utils/types.h"
#include "utils/jobcategorystrings.h"
#include "utils/rs_utils.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QDebug>

StationFreeRSModel::StationFreeRSModel(sqlite3pp::database &db, QObject *parent) :
    QAbstractTableModel(parent),
    sortCol(RSNameCol),
    mDb(db)
{
}

QVariant StationFreeRSModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case RSNameCol:
            return tr("Name");
        case FreeFromTimeCol:
            return tr("Free from");
        case FreeUpToTimeCol:
            return tr("Up to");
        case FromJobCol:
            return tr("Job A");
        case ToJobCol:
            return tr("Job B");
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

int StationFreeRSModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int StationFreeRSModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant StationFreeRSModel::data(const QModelIndex &idx, int role) const
{
    if (role != Qt::DisplayRole || !idx.isValid() || idx.row() >= m_data.size()
        || idx.column() >= NCols)
        return QVariant();

    const Item &item = m_data.at(idx.row());

    switch (idx.column())
    {
    case RSNameCol:
        return item.name;
    case FreeFromTimeCol:
        return item.from;
    case FreeUpToTimeCol:
        return item.to;
    case FromJobCol:
        if (item.fromJob)
            return JobCategoryName::jobName(item.fromJob, item.fromJobCat);
        break;
    case ToJobCol:
        if (item.toJob)
            return JobCategoryName::jobName(item.toJob, item.toJobCat);
        break;
    }

    return QVariant();
}

const StationFreeRSModel::Item *StationFreeRSModel::getItemAt(int row) const
{
    if (row < m_data.size())
        return &m_data.at(row);
    return nullptr;
}

void StationFreeRSModel::setStation(db_id stId)
{
    m_stationId = stId;
    reloadData();
}

void StationFreeRSModel::setTime(QTime time)
{
    m_time = time;
    reloadData();
}

template <typename Container, typename ItemType, typename LessThan>
void insertSorted(Container &vec, const ItemType &item, const LessThan &cmp)
{
    std::ptrdiff_t len = vec.size();
    auto first         = vec.begin();

    while (len > 0)
    {
        std::ptrdiff_t half = len >> 1;
        auto middle         = first + half;
        if (cmp(item, *middle))
            len = half;
        else
        {
            first = middle;
            ++first;
            len = len - half - 1;
        }
    }

    vec.insert(first, item);
}

void StationFreeRSModel::reloadData()
{
    beginResetModel();

    m_data.clear();
    QHash<db_id, Item> tempLookup;

    query q(mDb,
            "SELECT coupling.rs_id,rs_list.number,rs_models.name,rs_models.suffix,rs_models.type,"
            "MAX(stops.arrival),stops.job_id,jobs.category,stops.id"
            " FROM coupling"
            " JOIN stops ON stops.id=coupling.stop_id"
            " JOIN jobs ON jobs.id=stops.job_id"
            " JOIN rs_list ON rs_list.id=coupling.rs_id"
            " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id"
            " WHERE stops.station_id=? AND stops.arrival<=?" // Less than OR equal (this includes RS
                                                             // uncoupled exactly at that time)
            " GROUP BY coupling.rs_id"
            " HAVING coupling.operation=0");
    q.bind(1, m_stationId);
    q.bind(2, m_time);

    // Select uncoupled before m_time
    for (auto r : q)
    {
        Item item;
        item.rsId               = r.get<db_id>(0);

        int number              = r.get<int>(1);
        int modelNameLen        = sqlite3_column_bytes(q.stmt(), 2);
        const char *modelName   = reinterpret_cast<char const *>(sqlite3_column_text(q.stmt(), 2));

        int modelSuffixLen      = sqlite3_column_bytes(q.stmt(), 3);
        const char *modelSuffix = reinterpret_cast<char const *>(sqlite3_column_text(q.stmt(), 3));
        RsType type             = RsType(sqlite3_column_int(q.stmt(), 4));

        item.from               = r.get<QTime>(5);
        item.fromJob            = r.get<db_id>(6);
        item.fromJobCat         = JobCategory(r.get<int>(7));
        item.fromStopId         = r.get<db_id>(8);
        item.name = rs_utils::formatNameRef(modelName, modelNameLen, number, modelSuffix,
                                            modelSuffixLen, type);

        tempLookup.insert(item.rsId, item);
    }

    q.prepare(
      "SELECT coupling.rs_id,rs_list.number,rs_models.name,rs_models.suffix,rs_models.type,"
      "MIN(stops.arrival),stops.job_id,jobs.category,stops.id"
      " FROM coupling"
      " JOIN stops ON stops.id=coupling.stop_id"
      " JOIN jobs ON jobs.id=stops.job_id"
      " JOIN rs_list ON rs_list.id=coupling.rs_id"
      " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id"
      " WHERE stops.station_id=? AND stops.arrival>?" // Greater than NOT equal (this exclude RS
                                                      // coupled at exactly that time)
      " GROUP BY coupling.rs_id"
      " HAVING coupling.operation=1");
    q.bind(1, m_stationId);
    q.bind(2, m_time);

    // Select coupled after m_time
    for (auto r : q)
    {
        db_id rsId = r.get<db_id>(0);
        Item &item = tempLookup[rsId]; // Create entry if key doesn't exist
        if (!item.rsId)
        {
            item.rsId        = rsId;

            int number       = r.get<int>(1);
            int modelNameLen = sqlite3_column_bytes(q.stmt(), 2);
            const char *modelName =
              reinterpret_cast<char const *>(sqlite3_column_text(q.stmt(), 2));

            int modelSuffixLen = sqlite3_column_bytes(q.stmt(), 3);
            const char *modelSuffix =
              reinterpret_cast<char const *>(sqlite3_column_text(q.stmt(), 3));
            RsType type = RsType(sqlite3_column_int(q.stmt(), 4));

            item.name   = rs_utils::formatNameRef(modelName, modelNameLen, number, modelSuffix,
                                                  modelSuffixLen, type);
        }
        item.to       = r.get<QTime>(5);
        item.toJob    = r.get<db_id>(6);
        item.toJobCat = JobCategory(r.get<int>(7));
        item.toStopId = r.get<db_id>(8);
    }

    m_data.reserve(tempLookup.size());

    // Insert and sort
    switch (sortCol)
    {
    default:
    case RSNameCol:
    {
        class NameLessThan
        {
        public:
            inline bool operator()(const Item &lhs, const Item &rhs) const
            {
                return lhs.name < rhs.name;
            }
        };

        for (const Item &it : qAsConst(tempLookup))
        {
            insertSorted(m_data, it, NameLessThan());
        }
        break;
    }
    case FreeFromTimeCol:
    {
        class FromTimeLessThan
        {
        public:
            inline bool operator()(const Item &lhs, const Item &rhs) const
            {
                return lhs.from < rhs.from || (lhs.from == rhs.from && lhs.name < rhs.name);
            }
        };

        for (const Item &it : qAsConst(tempLookup))
        {
            insertSorted(m_data, it, FromTimeLessThan());
        }
        break;
    }
    case FreeUpToTimeCol:
    {
        class UpToTimeLessThan
        {
        public:
            inline bool operator()(const Item &lhs, const Item &rhs) const
            {
                return lhs.to < rhs.to || (lhs.to == rhs.to && lhs.name < rhs.name);
            }
        };

        for (const Item &it : qAsConst(tempLookup))
        {
            insertSorted(m_data, it, UpToTimeLessThan());
        }
        break;
    }
    case FromJobCol:
    {
        class FromJobLessThan
        {
        public:
            inline bool operator()(const Item &lhs, const Item &rhs) const
            {
                return lhs.fromJob < rhs.fromJob
                       || (lhs.fromJob == rhs.fromJob && lhs.name < rhs.name);
            }
        };

        for (const Item &it : qAsConst(tempLookup))
        {
            insertSorted(m_data, it, FromJobLessThan());
        }
        break;
    }
    case ToJobCol:
    {
        class ToJobLessThan
        {
        public:
            inline bool operator()(const Item &lhs, const Item &rhs) const
            {
                return lhs.toJob < rhs.toJob || (lhs.toJob == rhs.toJob && lhs.name < rhs.name);
            }
        };

        for (const Item &it : qAsConst(tempLookup))
        {
            insertSorted(m_data, it, ToJobLessThan());
        }
        break;
    }
    }

    m_data.squeeze();

    endResetModel();
}

int StationFreeRSModel::getSortCol() const
{
    return sortCol;
}

db_id StationFreeRSModel::getStationId() const
{
    return m_stationId;
}

QString StationFreeRSModel::getStationName() const
{
    query q(mDb, "SELECT name FROM stations WHERE id=?");
    q.bind(1, m_stationId);
    q.step();
    return q.getRows().get<QString>(0);
}

QTime StationFreeRSModel::getTime() const
{
    return m_time;
}

StationFreeRSModel::ErrorCodes StationFreeRSModel::getNextOpTime(QTime &time)
{
    ErrorCodes err = NoError;

    query q_getNextOpTime(mDb, "SELECT coupling.rs_id, MIN(stops.arrival) FROM coupling"
                               " JOIN stops ON stops.id=coupling.stop_id"
                               " WHERE stops.station_id=? AND stops.arrival>?");
    q_getNextOpTime.bind(1, m_stationId);
    q_getNextOpTime.bind(2, m_time);

    if (q_getNextOpTime.step() == SQLITE_ROW)
    {
        auto r = q_getNextOpTime.getRows();
        if (r.column_type(1) != SQLITE_NULL)
        {
            time = r.get<QTime>(1);
        }
        else
        {
            // There aren't operations next to m_time
            err = NoOperationFound;
        }
    }
    else
    {
        // Error
        qDebug() << __PRETTY_FUNCTION__ << "DB Error:" << mDb.error_code() << mDb.error_msg()
                 << mDb.extended_error_code();
        err = DBError;
    }

    q_getNextOpTime.reset();
    return err;
}

StationFreeRSModel::ErrorCodes StationFreeRSModel::getPrevOpTime(QTime &time)
{
    // TODO: if on last/first operation increment by 1 to see after/before prev, last operation
    ErrorCodes err = NoError;

    query q_getPrevOpTime(mDb, "SELECT coupling.rs_id, MAX(stops.arrival) FROM coupling"
                               " JOIN stops ON stops.id=coupling.stop_id"
                               " WHERE stops.station_id=? AND stops.arrival<?");
    q_getPrevOpTime.bind(1, m_stationId);
    q_getPrevOpTime.bind(2, m_time);

    if (q_getPrevOpTime.step() == SQLITE_ROW)
    {
        auto r = q_getPrevOpTime.getRows();
        if (r.column_type(1) != SQLITE_NULL)
        {
            time = r.get<QTime>(1);
        }
        else
        {
            // There aren't operations previous to m_time
            // But because RS uncoupled before m_time are taken with 'less than OR equal'
            // we should show also the situation before this time.

            // err = NoOperationFound;

            // Fake operation at first morning
            time = QTime(0, 0);
        }
    }
    else
    {
        // Error
        qDebug() << __PRETTY_FUNCTION__ << "DB Error:" << mDb.error_code() << mDb.error_msg()
                 << mDb.extended_error_code();
        err = DBError;
    }

    q_getPrevOpTime.reset();
    return err;
}

bool StationFreeRSModel::sortByColumn(int col)
{
    if (col == sortCol || col < 0 || col >= NCols)
        return false;

    sortCol = col;

    beginResetModel();
    switch (sortCol)
    {
    default:
    case RSNameCol:
    {
        class CompName : public std::binary_function<Item, Item, bool>
        {
        public:
            inline bool operator()(const Item &l, const Item &r)
            {
                return l.name < r.name;
            }
        };

        std::sort(m_data.begin(), m_data.end(), CompName());
        break;
    }
    case FreeFromTimeCol:
    {
        class CompFrom : public std::binary_function<Item, Item, bool>
        {
        public:
            inline bool operator()(const Item &l, const Item &r)
            {
                if (l.from == r.from)
                    return l.name < r.name;
                return l.from < r.from;
            }
        };

        std::sort(m_data.begin(), m_data.end(), CompFrom());
        break;
    }
    case FreeUpToTimeCol:
    {
        class CompTo : public std::binary_function<Item, Item, bool>
        {
        public:
            inline bool operator()(const Item &l, const Item &r)
            {
                if (l.to == r.to)
                    return l.name < r.name;
                return l.to < r.to;
            }
        };

        std::sort(m_data.begin(), m_data.end(), CompTo());
        break;
    }
    case FromJobCol:
    {
        class CompJobFrom : public std::binary_function<Item, Item, bool>
        {
        public:
            inline bool operator()(const Item &l, const Item &r)
            {
                if (l.fromJob == r.fromJob)
                    return l.name < r.name;
                return l.fromJob < r.fromJob;
            }
        };

        std::sort(m_data.begin(), m_data.end(), CompJobFrom());
        break;
    }
    case ToJobCol:
    {
        class CompJobTo : public std::binary_function<Item, Item, bool>
        {
        public:
            inline bool operator()(const Item &l, const Item &r)
            {
                if (l.toJob == r.toJob)
                    return l.name < r.name;
                return l.toJob < r.toJob;
            }
        };

        std::sort(m_data.begin(), m_data.end(), CompJobTo());
        break;
    }
    }
    endResetModel();

    return true;
}
