#include "railwaynodemodel.h"

#include "app/session.h" //TODO: needed?
#include "lines/linestorage.h"

#include <QCoreApplication>
#include <QEvent>
#include "utils/worker_event_types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/model_roles.h"
#include "utils/kmutils.h"

#include <QDebug>

class RailwayNodeModelResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::RailwayNodeModelResult);
    inline RailwayNodeModelResultEvent() :QEvent(_Type) {}

    QVector<RailwayNodeModel::Item> items;
    int firstRow;
};

//ERRORMSG: show other errors
static constexpr char
errorRSInUseCannotDelete[] = QT_TRANSLATE_NOOP("RailwayNodeModel",
                                               "Rollingstock item <b>%1</b> is used in some jobs so it cannot be removed.\n"
                                               "If you wish to remove it, please first remove it from its jobs.");

RailwayNodeModel::RailwayNodeModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(500, db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize),
    stationOrLineId(0),
    m_mode(RailwayNodeMode::StationLinesMode)
{
    sortColumn = KmCol;
}

bool RailwayNodeModel::event(QEvent *e)
{
    if(e->type() == RailwayNodeModelResultEvent::_Type)
    {
        RailwayNodeModelResultEvent *ev = static_cast<RailwayNodeModelResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

QVariant RailwayNodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case LineOrStationCol:
                return m_mode == RailwayNodeMode::StationLinesMode ? tr("Line") : tr("Station");
            case KmCol:
                return tr("Km");
            case DirectionCol:
                return tr("Direction");
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

int RailwayNodeModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int RailwayNodeModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant RailwayNodeModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<RailwayNodeModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const Item& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case LineOrStationCol:
            return item.lineOrStationName;
        case KmCol:
            return utils::kmNumToText(item.kmInMeters);
        case DirectionCol:
            return DirectionNames::name(item.direction);
        }
        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case LineOrStationCol:
            return item.lineOrStationId;
        case KmCol:
            return item.kmInMeters;
        case DirectionCol:
            return int(item.direction);
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        if(idx.column() == KmCol)
            return Qt::AlignRight + Qt::AlignVCenter;
        break;
    }
    case DIRECTION_ROLE:
        return int(item.direction);
    }

    return QVariant();
}

bool RailwayNodeModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols
            || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    Item &item = cache[row - cacheFirstRow];

    switch (role)
    {
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case KmCol:
        {
            bool ok = false;
            int kmInMeters = value.toInt(&ok);
            if(!setKm(item, kmInMeters))
                return false;
            break;
        }
        case DirectionCol:
        {
            Direction dir = Direction(value.toBool());
            if(!setDirection(item, dir))
                return false;
            break;
        }
        default:
            return false;
        }
        break;
    }
    case DIRECTION_ROLE:
    {
        bool ok = false;
        Direction dir = Direction(value.toInt(&ok));
        if(!ok || !setDirection(item, dir))
            return false;
        break;
    }
    }

    emit dataChanged(idx, idx);
    return true;
}

Qt::ItemFlags RailwayNodeModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    f.setFlag(Qt::ItemIsEditable);

    return f;
}

void RailwayNodeModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void RailwayNodeModel::refreshData(bool forceUpdate)
{
    if(!mDb.db())
        return;

    query q(mDb, m_mode == RailwayNodeMode::StationLinesMode ?
                "SELECT COUNT(1) FROM railways WHERE stationId=?" :
                "SELECT COUNT(1) FROM railways WHERE lineId=?");
    q.bind(1, stationOrLineId);
    q.step();
    const int count = q.getRows().get<int>(0);
    if(count != totalItemsCount)
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

void RailwayNodeModel::fetchRow(int row)
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
    //qDebug() << "Requested:" << row << "From:" << firstPendingRow;

    QVariant val;
    int valRow = 0;
    //    RSItem *item = nullptr;

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

void RailwayNodeModel::internalFetch(int first, int sortCol, int valRow, const QVariant& val)
{
    query q(mDb);

    int offset = first - valRow + curPage * ItemsPerPage;
    bool reverse = false;

    //    if(valRow > first) TODO: enable
    //    {
    //        offset = 0;
    //        reverse = true;
    //    }

    //qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    const char *orderCol;

    QByteArray sql;
    if(m_mode == RailwayNodeMode::StationLinesMode)
    {
        sql = "SELECT r.id,r.lineId,r.pos_meters,r.direction,lines.name"
              " FROM railways r"
              " LEFT JOIN lines ON lines.id=r.lineId"
              " WHERE r.stationId=?3";
    }
    else
    {
        sql = "SELECT r.id,r.stationId,r.pos_meters,r.direction,stations.name"
              " FROM railways r"
              " LEFT JOIN stations ON stations.id=r.stationId"
              " WHERE r.lineId=?3";
    }

    switch (sortCol)
    {
    case LineOrStationCol:
    {
        orderCol = m_mode == RailwayNodeMode::StationLinesMode ?
                    "lines.name" :
                    "stations.name";
        break;
    }
    case KmCol:
    {
        orderCol = "r.pos_meters";
        break;
    }
    case DirectionCol:
    {
        orderCol = m_mode == RailwayNodeMode::StationLinesMode ?
                    "r.direction,lines.name" :
                    "r.direction,stations.name";
        break;
    }
    }

    //    if(val.isValid())
    //    {
    //        sql += " WHERE ";
    //        sql += orderCol;
    //        if(reverse)
    //            sql += "<?3";
    //        else
    //            sql += ">?3";
    //    }

    sql += " ORDER BY ";
    sql += orderCol;

    if(reverse)
        sql += " DESC";

    sql += " LIMIT ?1";
    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);
    q.bind(3, stationOrLineId);

    //    if(val.isValid())
    //    {
    //        switch (sortCol)
    //        {
    //        case Model:
    //        {
    //            q.bind(3, val.toString());
    //            break;
    //        }
    //        }
    //    }

    QVector<Item> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    if(reverse)
    {
        int i = BatchSize - 1;

        for(; it != end; ++it)
        {
            auto r = *it;
            Item &item = vec[i];
            item.nodeId = r.get<db_id>(0);
            item.lineOrStationId = r.get<db_id>(1);
            item.kmInMeters = r.get<int>(2);
            item.direction = Direction(r.get<int>(3));
            item.lineOrStationName = r.get<QString>(4);
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
            Item &item = vec[i];
            item.nodeId = r.get<db_id>(0);
            item.lineOrStationId = r.get<db_id>(1);
            item.kmInMeters = r.get<int>(2);
            item.direction = Direction(r.get<int>(3));
            item.lineOrStationName = r.get<QString>(4);
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    RailwayNodeModelResultEvent *ev = new RailwayNodeModelResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void RailwayNodeModel::handleResult(const QVector<Item>& items, int firstRow)
{
    if(firstRow == cacheFirstRow + cache.size())
    {
        //qDebug() << "RES: appending First:" << cacheFirstRow;
        cache.append(items);
        if(cache.size() > ItemsPerPage)
        {
            const int extra = cache.size() - ItemsPerPage; //Round up to BatchSize
            const int remainder = extra % BatchSize;
            const int n = remainder ? extra + BatchSize - remainder : extra;
            //qDebug() << "RES: removing last" << n;
            cache.remove(0, n);
            cacheFirstRow += n;
        }
    }
    else
    {
        if(firstRow + items.size() == cacheFirstRow)
        {
            //qDebug() << "RES: prepending First:" << cacheFirstRow;
            QVector<Item> tmp = items;
            tmp.append(cache);
            cache = tmp;
            if(cache.size() > ItemsPerPage)
            {
                const int n = cache.size() - ItemsPerPage;
                cache.remove(ItemsPerPage, n);
                //qDebug() << "RES: removing first" << n;
            }
        }
        else
        {
            //qDebug() << "RES: replacing";
            cache = items;
        }
        cacheFirstRow = firstRow;
        //qDebug() << "NEW First:" << cacheFirstRow;
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

    //qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

void RailwayNodeModel::setSortingColumn(int col)
{
    if(sortColumn == col || col >= NCols)
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

bool RailwayNodeModel::getFieldData(int row, int col, db_id &idOut, QString &nameOut) const
{
    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size() || col != LineOrStationCol)
        return false;

    const Item& item = cache[row - cacheFirstRow];
    idOut = item.lineOrStationId;
    nameOut = item.lineOrStationName;

    return true;
}

bool RailwayNodeModel::validateData(int /*row*/, int /*col*/, db_id /*id*/, const QString &/*name*/)
{
    return true;
}

bool RailwayNodeModel::setFieldData(int row, int col, db_id id, const QString &name)
{
    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size() || col != LineOrStationCol)
        return false;

    Item& item = cache[row - cacheFirstRow];
    if(setLineOrStation(item, id, name))
    {
        QModelIndex first = index(row, RailwayNodeModel::LineOrStationCol);
        QModelIndex last = index(row, RailwayNodeModel::KmCol);
        emit dataChanged(first, last);
        return true;
    }
    return false;
}

void RailwayNodeModel::setMode(db_id id, RailwayNodeMode mode)
{
    stationOrLineId = id;
    m_mode = mode;
    clearCache();
    refreshData();
}

db_id RailwayNodeModel::addItem(db_id lineOrStationId, int *outRow)
{
    db_id nodeId = 0;

    db_id lineId;
    db_id stationId;

    if(m_mode == RailwayNodeMode::StationLinesMode)
    {
        lineId = lineOrStationId;
        stationId = stationOrLineId;
    }else{
        lineId = stationOrLineId;
        stationId = lineOrStationId;
    }
    int kmInMeters = getLineMaxKmInMeters(lineId);

    command cmd(mDb, "INSERT INTO railways (id,lineId,stationId,pos_meters,direction)"
                     "VALUES(NULL,?,?,?,0)");
    cmd.bind(1, lineId);
    cmd.bind(2, stationId);
    cmd.bind(3, kmInMeters);

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret = cmd.execute();
    if(ret == SQLITE_OK)
    {
        nodeId = mDb.last_insert_rowid();
    }
    sqlite3_mutex_leave(mutex);

    if(ret == SQLITE_CONSTRAINT_UNIQUE)
    {
        //There is already an entry with same line/station
        //ERRORMSG
        nodeId = 0;
    }
    else if(ret != SQLITE_OK)
    {
        qDebug() << "RailwayNodeModel Error adding:" << ret << mDb.error_msg() << mDb.error_code() << mDb.extended_error_code();
    }

    if(nodeId)
    {
        Session->mLineStorage->addStationToLine(lineId, stationId);
        Session->mLineStorage->redrawLine(lineId);
        refreshData(); //Recalc row count
        switchToPage(0); //Reset to first page and so it is shown as first row
    }

    if(outRow)
        *outRow = nodeId ? 0 : -1; //Empty model is always the first

    return nodeId;
}

bool RailwayNodeModel::removeItem(int row)
{
    if(row >= curItemCount || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    const Item &item = cache.at(row - cacheFirstRow);
    command cmd(mDb, "DELETE FROM railways WHERE id=?");
    cmd.bind(1, item.nodeId);
    int ret = cmd.execute();

    db_id lineId;
    db_id stationId;

    if(m_mode == RailwayNodeMode::StationLinesMode)
    {
        lineId = item.lineOrStationId;
        stationId = stationOrLineId;
    }else{
        lineId = stationOrLineId;
        stationId = item.lineOrStationId;
    }

    if(ret != SQLITE_OK)
    {
        qDebug() << Q_FUNC_INFO << "RailwayNodeModel Error:" << ret << mDb.error_msg() << "Node:" << item.nodeId;
        int err = mDb.extended_error_code();
        if(err == SQLITE_CONSTRAINT_TRIGGER)
            qDebug() << "Cannot remove station" << stationId << "IS IN USE";
        return false;
    }

    if(lineId && stationId)
    {
        Session->mLineStorage->removeStationFromLine(lineId, stationId);
        Session->mLineStorage->redrawLine(lineId);
    }

    refreshData();
    return true;
}

bool RailwayNodeModel::setLineOrStation(Item &item, db_id id, const QString &name)
{
    if(item.lineOrStationId == id)
        return true;

    if(!id)
        return false;

    command q_setLine(mDb, m_mode == RailwayNodeMode::StationLinesMode ?
                          "UPDATE railways SET lineId=? WHERE id=?" :
                          "UPDATE railways SET stationId=? WHERE id=?");
    q_setLine.bind(1, item.lineOrStationId);
    q_setLine.bind(2, item.nodeId);
    int ret = q_setLine.execute();

    if(ret != SQLITE_OK)
    {
        int err = mDb.extended_error_code();
        if(err == SQLITE_CONSTRAINT_UNIQUE)
        {
            //The requested stations already is in this line (prevent duplicates)
            //ERRORMSG: show messagebox
            qDebug() << Q_FUNC_INFO << "Error: station - line couple IS NOT UNIQUE";
        }
        return false;
    }

    LineStorage *lineStorage = Session->mLineStorage;
    db_id lineId = 0;
    if(m_mode == RailwayNodeMode::LineStationsMode)
    {
        //Remove previous station if any
        lineId = stationOrLineId;
        if(item.lineOrStationId != 0)
            lineStorage->removeStationFromLine(lineId, item.lineOrStationId);

        item.lineOrStationId = id; //Set new station
        if(item.lineOrStationId != 0)
            lineStorage->addStationToLine(lineId, item.lineOrStationId);
    }
    else
    {
        lineId = id;
        int kmInMeters = getLineMaxKmInMeters(lineId);
        setNodeKm(item.nodeId, kmInMeters);

        //Remove from previous line if any
        if(item.lineOrStationId != 0)
        {
            lineStorage->removeStationFromLine(item.lineOrStationId, stationOrLineId);
            lineStorage->redrawLine(item.lineOrStationId); //Redraw old line
        }

        item.kmInMeters = kmInMeters;
        item.lineOrStationId = lineId;
        if(item.lineOrStationId != 0)
            lineStorage->addStationToLine(item.lineOrStationId, stationOrLineId);
    }

    item.lineOrStationName = name;
    if(lineId)
        lineStorage->redrawLine(lineId); //Redraw new line

    //This migth change also km, and when sorting by direction the name is also taken in account
    //so reload data in every case
    //This row has now changed position so we need to invalidate cache
    //HACK: we emit dataChanged for this index (that doesn't exist anymore)
    //but the view will trigger fetching at same scroll position so it is enough
    cache.clear();
    cacheFirstRow = 0;

    return true;
}

bool RailwayNodeModel::setKm(Item &item, int kmInMeters)
{
    if(item.kmInMeters == kmInMeters)
        return true;

    db_id lineId = m_mode == RailwayNodeMode::StationLinesMode ? item.lineOrStationId : stationOrLineId;
    if(!lineId)
        return false; //Cannot set 'km' before setting 'line'

    //Invalid (km < 0) or there's another station already at that km
    if(kmInMeters < 0)
        return false; //ERRORMSG: show message box

    query q_lineHasStAtKm(mDb, "SELECT stationId FROM railways WHERE lineId=? AND pos_meters=?");
    q_lineHasStAtKm.bind(1, lineId);
    q_lineHasStAtKm.bind(2, kmInMeters);
    int ret = q_lineHasStAtKm.step();

    //There's already a station at that km
    if(ret == SQLITE_ROW)
        return false;

    if(!setNodeKm(item.nodeId, kmInMeters))
        return false;

    item.kmInMeters = kmInMeters;

    //Update graph and jobs
    Session->mLineStorage->redrawLine(lineId);

    if(sortColumn == KmCol)
    {
        //This row has now changed position so we need to invalidate cache
        //HACK: we emit dataChanged for this index (that doesn't exist anymore)
        //but the view will trigger fetching at same scroll position so it is enough
        cache.clear();
        cacheFirstRow = 0;
    }

    return true;
}

bool RailwayNodeModel::setDirection(Item &item, Direction dir)
{
    if(item.direction == dir)
        return true;

    command q_setDirection  (mDb, "UPDATE railways SET direction=? WHERE id=?");
    q_setDirection.bind(1, int(dir));
    q_setDirection.bind(2, item.nodeId);
    int ret = q_setDirection.execute();
    if(ret != SQLITE_OK)
        return false;

    item.direction = dir;

    if(sortColumn == DirectionCol)
    {
        //This row has now changed position so we need to invalidate cache
        //HACK: we emit dataChanged for this index (that doesn't exist anymore)
        //but the view will trigger fetching at same scroll position so it is enough
        cache.clear();
        cacheFirstRow = 0;
    }

    return true;
}

int RailwayNodeModel::getLineMaxKmInMeters(db_id lineId)
{
    query q_getLineMaxKm  (mDb, "SELECT MAX(pos_meters) FROM railways WHERE lineId=?");
    q_getLineMaxKm.bind(1, lineId);
    q_getLineMaxKm.step();
    auto row = q_getLineMaxKm.getRows();

    int kmInMeters = 0;
    //If MAX(km) returns NULL leave 'km' = 0
    //Otherwise add +1 to last station km
    if(row.column_type(0) != SQLITE_NULL)
    {
        kmInMeters = row.get<int>(0) + 1000; //Add 1 km (1000 meters) to highest km in the line
    }
    return kmInMeters;
}

bool RailwayNodeModel::setNodeKm(db_id nodeId, int kmInMeters)
{
    command q_setKm(mDb, "UPDATE railways SET pos_meters=? WHERE id=?");
    q_setKm.bind(1, kmInMeters);
    q_setKm.bind(2, nodeId);
    int ret = q_setKm.execute();

    if(ret != SQLITE_OK)
    {
        qWarning() << "RailwayNodeModel Error: setting km for node:" << nodeId << mDb.error_msg() << ret;
        return false;
    }
    return true;
}
