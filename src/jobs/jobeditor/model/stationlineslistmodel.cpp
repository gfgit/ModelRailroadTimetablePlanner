#include "stationlineslistmodel.h"

#include <QCoreApplication>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/rs_types_names.h"

#include "utils/worker_event_types.h"

#include <QDebug>

class StationLinesListModelResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::StationLinesListModelResult);
    inline StationLinesListModelResultEvent() :QEvent(_Type) {}

    QVector<StationLinesListModel::LineItem> items;
    int firstRow;
};

StationLinesListModel::StationLinesListModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(50, db, parent),
    m_stationId(0),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
    sortColumn = Name;
}

bool StationLinesListModel::event(QEvent *e)
{
    if(e->type() == StationLinesListModelResultEvent::_Type)
    {
        StationLinesListModelResultEvent *ev = static_cast<StationLinesListModelResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

QVariant StationLinesListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case Name:
                return tr("Name");
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

int StationLinesListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int StationLinesListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant StationLinesListModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    //qDebug() << "Data:" << idx.row();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<StationLinesListModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const LineItem& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case Name:
            return item.name;
        }

        break;
    }
    }

    return QVariant();
}

qint64 StationLinesListModel::recalcTotalItemCount()
{
    qint64 count = 0;
    if(m_stationId)
    {
        query q(mDb, "SELECT COUNT(1) FROM railways WHERE stationId=?");
        q.bind(1, m_stationId);
        q.step();
        count = q.getRows().get<qint64>(0);
    }
    return count;
}

void StationLinesListModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void StationLinesListModel::fetchRow(int row)
{
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
    //    RSOwner *item = nullptr;

    //    if(cache.size())
    //    {
    //        if(firstPendingRow >= cacheFirstRow + cache.size())
    //        {
    //            valRow = cacheFirstRow + cache.size();
    //            item = &cache.last();
    //        }
    //        else if(firstPendingRow > (cacheFirstRow - firstPendingRow))
    //        {
    //            valRow = cacheFirstRow;
    //            item = &cache.first();
    //        }
    //    }

    /*switch (sortCol) TODO: use val in WHERE clause
    {
    case Name:
    {
        if(item)
        {
            val = item->name;
        }
        break;
    }
        //No data hint for TypeCol column
    }*/

    //TODO: use a custom QRunnable
    //    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection,
    //                              Q_ARG(int, firstPendingRow), Q_ARG(int, sortCol),
    //                              Q_ARG(int, valRow), Q_ARG(QVariant, val));
    internalFetch(firstPendingRow, sortColumn, val.isNull() ? 0 : valRow, val);
}

void StationLinesListModel::internalFetch(int first, int sortCol, int valRow, const QVariant& val)
{
    query q(mDb);

    int offset = first - valRow + curPage * ItemsPerPage;
    bool reverse = false;

    //    if(valRow > first)
    //    {
    //        offset = 0;
    //        reverse = true;
    //    }

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    //const char *whereCol;

    QByteArray sql = "SELECT railways.lineId,lines.name FROM railways"
                     " JOIN lines ON lines.id=railways.lineId"
                     " WHERE railways.stationId=?3"
                     " ORDER BY lines.name"; //TODO: support valRow
    //    switch (sortCol)
    //    {
    //    case Name:
    //    {
    //        whereCol = "name"; //Order by 2 columns, no where clause
    //        break;
    //    }
    //    }

    //    if(val.isValid())
    //    {
    //        sql += " WHERE ";
    //        sql += whereCol;
    //        if(reverse)
    //            sql += "<?3";
    //        else
    //            sql += ">?3";
    //    }

    //    sql += " ORDER BY ";
    //    sql += whereCol;

    //    if(reverse)
    //        sql += " DESC";

    sql += " LIMIT ?1";
    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);
    q.bind(3, m_stationId);

    if(val.isValid())
    {
        switch (sortCol)
        {
        case Name:
        {
            q.bind(3, val.toString());
            break;
        }
        }
    }

    QVector<LineItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    if(reverse)
    {
        int i = BatchSize - 1;

        for(; it != end; ++it)
        {
            auto r = *it;
            LineItem &item = vec[i];
            item.lineId = r.get<db_id>(0);
            item.name = r.get<QString>(1);
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
            LineItem &item = vec[i];
            item.lineId = r.get<db_id>(0);
            item.name = r.get<QString>(1);
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    StationLinesListModelResultEvent *ev = new StationLinesListModelResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void StationLinesListModel::handleResult(const QVector<LineItem>& items, int firstRow)
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
            qDebug() << "RES: removing last" << n;
            cache.remove(0, n);
            cacheFirstRow += n;
        }
    }
    else
    {
        if(firstRow + items.size() == cacheFirstRow)
        {
            qDebug() << "RES: prepending First:" << cacheFirstRow;
            QVector<LineItem> tmp = items;
            tmp.append(cache);
            cache = tmp;
            if(cache.size() > ItemsPerPage)
            {
                const int n = cache.size() - ItemsPerPage;
                cache.remove(ItemsPerPage, n);
                qDebug() << "RES: removing first" << n;
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
    emit resultsReady();

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

void StationLinesListModel::setSortingColumn(int /*col*/)
{
    //Only sort by name
}

void StationLinesListModel::setStationId(db_id stationId)
{
    m_stationId = stationId;
    refreshData(true);
}

int StationLinesListModel::getLineRow(db_id lineId)
{
    if(!m_stationId)
        return -1;

    query q(mDb, "SELECT 1 FROM railways WHERE lineId=? AND stationId=? LIMIT 1");
    q.bind(1, lineId);
    q.bind(2, m_stationId);
    if(q.step() != SQLITE_ROW)
        return -1; //This line doesn't pass in this station

    q.prepare("SELECT COUNT(1) FROM railways"
              " JOIN lines l1 ON l1.id=railways.lineId"
              " JOIN lines l2 ON l2.id=?"
              " WHERE railways.stationId=? AND l1.name<l2.name");
    q.bind(1, lineId);
    q.bind(2, m_stationId);

    return q.getRows().get<int>(0);
}

db_id StationLinesListModel::getLineIdAt(int row)
{
    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return 0;
    return cache.at(row - cacheFirstRow).lineId;
}
