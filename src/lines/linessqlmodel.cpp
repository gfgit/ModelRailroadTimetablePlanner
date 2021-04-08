#include "linessqlmodel.h"
#include "app/session.h"

#include <QCoreApplication>
#include <QEvent>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/worker_event_types.h"
#include "utils/model_roles.h"

#include "linestorage.h"

#include <QDebug>

class LinesSQLModelResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::LinesModelResult);
    inline LinesSQLModelResultEvent() : QEvent(_Type) {}

    QVector<LinesSQLModel::LineItem> items;
    int firstRow;
};

LinesSQLModel::LinesSQLModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(500, db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
    sortColumn = LineNameCol;
    LineStorage *lines = Session->mLineStorage;
    connect(lines, &LineStorage::lineAdded, this, &LinesSQLModel::onLineAdded);
    connect(lines, &LineStorage::lineRemoved, this, &LinesSQLModel::onLineRemoved);
}

bool LinesSQLModel::event(QEvent *e)
{
    if(e->type() == LinesSQLModelResultEvent::_Type)
    {
        LinesSQLModelResultEvent *ev = static_cast<LinesSQLModelResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

QVariant LinesSQLModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case LineNameCol:
                return tr("Name");
            case LineMaxSpeedKmHCol:
                return tr("Max. Speed");
            case LineTypeCol:
                return tr("Type");
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

int LinesSQLModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int LinesSQLModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant LinesSQLModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<LinesSQLModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const LineItem& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case LineNameCol:
            return item.name;
        case LineMaxSpeedKmHCol:
            return tr("%1 km/h").arg(item.maxSpeedKmH);
        }
        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case LineNameCol:
            return item.name;
        case LineMaxSpeedKmHCol:
            return item.maxSpeedKmH;
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        if(idx.column() == LineMaxSpeedKmHCol)
        {
            return Qt::AlignVCenter + Qt::AlignRight;
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        if(idx.column() == LineTypeCol)
        {
            return item.type == LineType::Electric ? Qt::Checked : Qt::Unchecked;
        }
        break;
    }
    }

    return QVariant();
}

bool LinesSQLModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    LineItem &item = cache[row - cacheFirstRow];
    QModelIndex first = idx;
    QModelIndex last = idx;

    switch (role)
    {
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case LineNameCol:
        {
            const QString newName = value.toString().simplified();
            if(!setName(item, newName))
                return false;
            break;
        }
        case LineMaxSpeedKmHCol:
        {
            bool ok = false;
            int speed = value.toInt(&ok);
            if(!ok || !setMaxSpeed(item, speed))
                return false;
            break;
        }
        default:
            break;
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        if(idx.column() == LineTypeCol)
        {
            LineType type = value.value<Qt::CheckState>() == Qt::Checked ? LineType::Electric : LineType::NonElectric;
            if(!setType(item, type))
                return false;
        }
        break;
    }
    default:
        break;
    }

    emit dataChanged(first, last);
    return true;
}

Qt::ItemFlags LinesSQLModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    if(idx.column() == LineTypeCol)
        f.setFlag(Qt::ItemIsUserCheckable); //LineTypeCol checkbox
    else
        f.setFlag(Qt::ItemIsEditable);

    return f;
}

void LinesSQLModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void LinesSQLModel::refreshData(bool forceUpdate)
{
    if(!mDb.db())
        return;

    emit itemsReady(-1, -1); //Notify we are about to refresh

    //TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM lines");
    q.step();
    const int count = q.getRows().get<int>(0);
    if(count != totalItemsCount || forceUpdate)
    {
        beginResetModel();

        clearCache();
        totalItemsCount = count;
        emit totalItemsCountChanged(totalItemsCount);

        //Round up division
        const int rem = count % ItemsPerPage;
        pageCount = count / ItemsPerPage + (rem != 0);
        emit pageCountChanged(pageCount);

        if(curPage >= pageCount)
        {
            switchToPage(pageCount - 1);
        }

        curItemCount = totalItemsCount ? (curPage == pageCount - 1 && rem) ? rem : ItemsPerPage : 0;

        endResetModel();
    }
}

void LinesSQLModel::setSortingColumn(int col)
{
    if(sortColumn == col || col >= NCols)
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

bool LinesSQLModel::removeLine(db_id lineId)
{
    return Session->mLineStorage->removeLine(lineId);
}

db_id LinesSQLModel::addLine(int *outRow, QString *errOut)
{
    db_id lineId = 0;
    if(outRow)
        *outRow = 0;
    if(!Session->mLineStorage->addLine(&lineId))
        return 0; //Error

    return lineId;
}

void LinesSQLModel::onLineAdded()
{
    refreshData(); //Recalc row count
    setSortingColumn(LineNameCol);
    switchToPage(0); //Reset to first page and so it is shown as first row
}

void LinesSQLModel::onLineRemoved()
{
    refreshData(); //Recalc row count
}

void LinesSQLModel::fetchRow(int row)
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


    //TODO: use a custom QRunnable
    //    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection,
    //                              Q_ARG(int, firstPendingRow), Q_ARG(int, sortCol),
    //                              Q_ARG(int, valRow), Q_ARG(QVariant, val));
    internalFetch(firstPendingRow, sortColumn, val.isNull() ? 0 : valRow, val);
}

void LinesSQLModel::internalFetch(int first, int sortCol, int valRow, const QVariant &val)
{
    query q(mDb);

    int offset = first - valRow + curPage * ItemsPerPage;
    bool reverse = false;

    if(valRow > first)
    {
        offset = 0;
        reverse = true;
    }

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    const char *whereCol = nullptr;

    QByteArray sql = "SELECT id,name,max_speed,type FROM lines";
    switch (sortCol)
    {
    case LineNameCol:
    {
        whereCol = "name"; //Order by 1 column, no where clause
        break;
    }
    case LineMaxSpeedKmHCol:
    {
        whereCol = "max_speed,name"; //Order by 2 columns, no where clause
        break;
    }
    case LineTypeCol:
    {
        whereCol = "type,max_speed,name"; //Order by 3 columns, no where clause
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

    if(val.isValid())
    {
        switch (sortCol)
        {
        case LineNameCol:
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
            item.maxSpeedKmH = r.get<int>(2);
            item.type = LineType(r.get<int>(3));
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
            item.maxSpeedKmH = r.get<int>(2);
            item.type = LineType(r.get<int>(3));
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    LinesSQLModelResultEvent *ev = new LinesSQLModelResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void LinesSQLModel::handleResult(const QVector<LinesSQLModel::LineItem> &items, int firstRow)
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
    emit itemsReady(firstRow, lastRow);

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

bool LinesSQLModel::setName(LinesSQLModel::LineItem &item, const QString &name)
{
    if(item.name == name || name.isEmpty())
        return false;

    command set_name(mDb, "UPDATE lines SET name=? WHERE id=?");
    set_name.bind(1, name);
    set_name.bind(2, item.lineId);
    int ret = set_name.execute();
    if(ret != SQLITE_OK)
    {
        qDebug() << "setName()" << ret << mDb.error_code() << mDb.extended_error_code() << mDb.error_msg();
    }

    item.name = name;

    //This row has now changed position so we need to invalidate cache
    //HACK: we emit dataChanged for this index (that doesn't exist anymore)
    //but the view will trigger fetching at same scroll position so it is enough
    cache.clear();
    cacheFirstRow = 0;

    emit Session->mLineStorage->lineNameChanged(item.lineId);

    return true;
}

bool LinesSQLModel::setMaxSpeed(LinesSQLModel::LineItem &item, int speed)
{
    if(item.maxSpeedKmH == speed || speed < 1 || speed > 9999)
        return false;

    command set_speed(mDb, "UPDATE lines SET max_speed=? WHERE id=?");
    set_speed.bind(1, speed);
    set_speed.bind(2, item.lineId);
    int ret = set_speed.execute();
    if(ret != SQLITE_OK)
    {
        qDebug() << "setMaxSpeed()" << ret << mDb.error_code() << mDb.extended_error_code() << mDb.error_msg();
    }

    item.maxSpeedKmH = speed;

    if(sortColumn != LineNameCol)
    {
        //This row has now changed position so we need to invalidate cache
        //HACK: we emit dataChanged for this index (that doesn't exist anymore)
        //but the view will trigger fetching at same scroll position so it is enough
        cache.clear();
        cacheFirstRow = 0;
    }

    return true;
}

bool LinesSQLModel::setType(LinesSQLModel::LineItem &item, LineType type)
{
    if(item.type == type)
        return false;

    command set_type(mDb, "UPDATE lines SET type=? WHERE id=?");
    set_type.bind(1, int(type));
    set_type.bind(2, item.lineId);
    int ret = set_type.execute();
    if(ret != SQLITE_OK)
    {
        qDebug() << "setType()" << ret << mDb.error_code() << mDb.extended_error_code() << mDb.error_msg();
    }

    item.type = type;

    if(sortColumn == LineTypeCol)
    {
        //This row has now changed position so we need to invalidate cache
        //HACK: we emit dataChanged for this index (that doesn't exist anymore)
        //but the view will trigger fetching at same scroll position so it is enough
        cache.clear();
        cacheFirstRow = 0;
    }

    return true;
}
