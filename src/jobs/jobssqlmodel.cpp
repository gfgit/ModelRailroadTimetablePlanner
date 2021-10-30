#include "jobssqlmodel.h"
#include "app/session.h"

#include <QCoreApplication>
#include <QEvent>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/worker_event_types.h"
#include "utils/model_roles.h"

#include "utils/jobcategorystrings.h"

#include <QDebug>

class JobsSQLModelResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::JobsModelResult);
    inline JobsSQLModelResultEvent() : QEvent(_Type) {}

    QVector<JobsSQLModel::JobItem> items;
    int firstRow;
};

JobsSQLModel::JobsSQLModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(500, db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
    sortColumn = IdCol;

    connect(Session, &MeetingSession::shiftNameChanged, this, &JobsSQLModel::clearCache_slot);
    connect(Session, &MeetingSession::shiftJobsChanged, this, &JobsSQLModel::clearCache_slot);
    connect(Session, &MeetingSession::stationNameChanged, this, &JobsSQLModel::clearCache_slot);
    connect(Session, &MeetingSession::jobChanged, this, &JobsSQLModel::clearCache_slot);

    connect(Session, &MeetingSession::jobAdded, this, &JobsSQLModel::onJobAddedOrRemoved);
    connect(Session, &MeetingSession::jobRemoved, this, &JobsSQLModel::onJobAddedOrRemoved);
}

bool JobsSQLModel::event(QEvent *e)
{
    if(e->type() == JobsSQLModelResultEvent::_Type)
    {
        JobsSQLModelResultEvent *ev = static_cast<JobsSQLModelResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

QVariant JobsSQLModel::headerData(int section, Qt::Orientation orientation, int role) const
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

int JobsSQLModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int JobsSQLModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant JobsSQLModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<JobsSQLModel *>(this)->fetchRow(row);

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

qint64 JobsSQLModel::recalcTotalItemCount()
{
    //TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM jobs");
    q.step();
    const qint64 count = q.getRows().get<int>(0);
    return count;
}

void JobsSQLModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void JobsSQLModel::setSortingColumn(int col)
{
    if(sortColumn == col || col == OriginSt || col == DestinationSt || col >= NCols)
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

void JobsSQLModel::clearCache_slot()
{
    clearCache();
    QModelIndex start = index(0, 0);
    QModelIndex end = index(curItemCount - 1, NCols - 1);
    emit dataChanged(start, end);
}

void JobsSQLModel::onJobAddedOrRemoved()
{
    refreshData(); //Recalc row count
}

void JobsSQLModel::fetchRow(int row)
{
    if(!mDb.db())
        return;

    if(firstPendingRow != -BatchSize)
        return; //Currently fetching another batch, wait for it to finish first

    if(row >= firstPendingRow && row < firstPendingRow + BatchSize)
        return; //Already fetching this batch

    if(row >= cacheFirstRow && row < cacheFirstRow + cache.size())
        return; //Already cached

    //TODO: abort fetching here

    const int remainder = row % BatchSize;
    firstPendingRow = row - remainder;
    qDebug() << "Requested:" << row << "From:" << firstPendingRow;

    QVariant val;
    int valRow = 0;


    //TODO: use a custom QRunnable
    //    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection,
    //                              Q_ARG(int, firstPendingRow), Q_ARG(int, sortCol),
    //                              Q_ARG(int, valRow), Q_ARG(QVariant, val));
    internalFetch(firstPendingRow, sortColumn, val.isNull() ? 0 : valRow, val);
}

void JobsSQLModel::internalFetch(int first, int sortCol, int valRow, const QVariant &val)
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

    QByteArray sql = "SELECT jobs.id, jobs.category, jobs.shiftId, jobshifts.name,"
                     "MIN(s1.departure), s1.stationId, MAX(s2.arrival), s2.stationId"
                     " FROM jobs"
                     " LEFT JOIN stops s1 ON s1.job_id=jobs.id"
                     " LEFT JOIN stops s2 ON s2.job_id=jobs.id"
                     " LEFT JOIN jobshifts ON jobshifts.id=jobs.shiftId";

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

    if(reverse)
    {
        int i = BatchSize - 1;

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

            i--;
        }
        if(i > -1)
            vec.remove(0, i + 1);
    }
    else
    {
        int i = 0;

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

            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    JobsSQLModelResultEvent *ev = new JobsSQLModelResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void JobsSQLModel::handleResult(const QVector<JobsSQLModel::JobItem> &items, int firstRow)
{
    if(firstRow == cacheFirstRow + cache.size())
    {
        qDebug() << "RES: appending First:" << cacheFirstRow;
        cache.append(items);
        if(cache.size() > ItemsPerPage)
        {
            const int extra = cache.size() - ItemsPerPage; //Round up to BatchSize
            const int remainder = extra % BatchSize;
            const int n = remainder ? extra + BatchSize - remainder : extra;
            qDebug() << "RES: removing first" << n;
            cache.remove(0, n);
            cacheFirstRow += n;
        }
    }
    else
    {
        if(firstRow + items.size() == cacheFirstRow)
        {
            qDebug() << "RES: prepending First:" << cacheFirstRow;
            QVector<JobItem> tmp = items;
            tmp.append(cache);
            cache = tmp;
            if(cache.size() > ItemsPerPage)
            {
                const int n = cache.size() - ItemsPerPage;
                cache.remove(ItemsPerPage, n);
                qDebug() << "RES: removing last" << n;
            }
        }
        else
        {
            qDebug() << "RES: replacing";
            cache = items;
        }
        cacheFirstRow = firstRow;
        qDebug() << "NEW First:" << cacheFirstRow;
    }

    firstPendingRow = -BatchSize;

    int lastRow = firstRow + items.count(); //Last row + 1 extra to re-trigger possible next batch
    if(lastRow >= curItemCount)
        lastRow = curItemCount -1; //Ok, there is no extra row so notify just our batch

    if(firstRow > 0)
        firstRow--; //Try notify also the row before because there might be another batch waiting so re-trigger it
    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(lastRow, NCols - 1);
    emit dataChanged(firstIdx, lastIdx);
    emit itemsReady(firstRow, lastRow);

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}
