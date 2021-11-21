#include "joblistmodel.h"

#include "utils/sqldelegate/pageditemmodelhelper_impl.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/model_roles.h"

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
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
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

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<JobListModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const JobItem& item = cache.at(row - cacheFirstRow);

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
        if(idx.column() == IdCol)
        {
            return Qt::AlignVCenter + Qt::AlignRight;
        }
        break;
    }
    }

    return QVariant();
}

void JobListModel::setSortingColumn(int col)
{
    if(sortColumn == col || col == OriginSt || col == DestinationSt || col >= NCols)
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

void JobListModel::onJobAddedOrRemoved()
{
    refreshData(); //Recalc row count
}

qint64 JobListModel::recalcTotalItemCount()
{
    //TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM jobs");
    q.step();
    const qint64 count = q.getRows().get<int>(0);
    return count;
}

void JobListModel::internalFetch(int first, int sortCol, int valRow, const QVariant &val)
{
    query q(mDb);

    query q_stationName(mDb, "SELECT name FROM stations WHERE id=?");

    int offset = first - valRow + curPage * ItemsPerPage;
    bool reverse = false;

    if(valRow > first)
    {
        offset = 0;
        reverse = true;
    }

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    const char *whereCol = nullptr;

    QByteArray sql = "SELECT jobs.id, jobs.category, jobs.shift_id, jobshifts.name,"
                     "MIN(s1.departure), s1.station_id, MAX(s2.arrival), s2.station_id"
                     " FROM jobs"
                     " LEFT JOIN stops s1 ON s1.job_id=jobs.id"
                     " LEFT JOIN stops s2 ON s2.job_id=jobs.id"
                     " LEFT JOIN jobshifts ON jobshifts.id=jobs.shift_id"
                     " GROUP BY jobs.id";

    switch (sortCol)
    {
    case IdCol:
    {
        whereCol = "jobs.id"; //Order by 1 column, no where clause
        break;
    }
    case Category:
    {
        whereCol = "jobs.category,jobs.id";
        break;
    }
    case ShiftCol:
    {
        whereCol = "jobshifts.name,s1.departure,jobs.id";
        break;
    }
    case OriginTime:
    {
        whereCol = "s1.departure,jobs.id";
        break;
    }
    case DestinationTime:
    {
        whereCol = "s2.arrival,jobs.id";
        break;
    }
    }

    if(val.isValid())
    {
        sql += " WHERE ";
        sql += whereCol;
        if(reverse)
            sql += "<?3";
        else
            sql += ">?3";
    }

    sql += " ORDER BY ";
    sql += whereCol;

    if(reverse)
        sql += " DESC";

    sql += " LIMIT ?1";
    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);

    //    if(val.isValid())
    //    {
    //        switch (sortCol)
    //        {
    //        case LineNameCol:
    //        {
    //            q.bind(3, val.toString());
    //            break;
    //        }
    //        }
    //    }

    QVector<JobItem> vec(BatchSize);

    //QString are implicitly shared, use QHash to temporary store them instead
    //of creating new ones for each JobItem
    QHash<db_id, QString> shiftHash;
    QHash<db_id, QString> stationHash;

    auto it = q.begin();
    const auto end = q.end();

    int i = reverse ? BatchSize - 1 : 0;
    const int increment = reverse ? -1 : 1;

    for(; it != end; ++it)
    {
        auto r = *it;
        JobItem &item = vec[i];
        item.jobId = r.get<db_id>(0);
        item.category = JobCategory(r.get<int>(1));
        item.shiftId = r.get<db_id>(2);

        if(item.shiftId)
        {
            auto shift = shiftHash.constFind(item.shiftId);
            if(shift == shiftHash.constEnd())
            {
                shift = shiftHash.insert(item.shiftId, r.get<QString>(3));
            }
            item.shiftName = shift.value();
        }

        item.originTime = r.get<QTime>(4);
        item.originStId = r.get<db_id>(5);
        item.destTime = r.get<QTime>(6);
        item.destStId = r.get<db_id>(7);

        if(item.originStId)
        {
            auto st = stationHash.constFind(item.originStId);
            if(st == stationHash.constEnd())
            {
                q_stationName.bind(1, item.originStId);
                q_stationName.step();
                st = stationHash.insert(item.originStId, q_stationName.getRows().get<QString>(0));
                q_stationName.reset();
            }
            item.origStName = st.value();
        }

        if(item.destStId)
        {
            auto st = stationHash.constFind(item.destStId);
            if(st == stationHash.constEnd())
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

    if(reverse && i > -1)
        vec.remove(0, i + 1);
    else if(i < BatchSize)
        vec.remove(i, BatchSize - i);

    postResult(vec, first);
}
