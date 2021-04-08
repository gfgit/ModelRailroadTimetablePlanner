#include "stationssqlmodel.h"
#include "app/session.h"

#include <QCoreApplication>
#include <QEvent>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/worker_event_types.h"
#include "utils/model_roles.h"
#include "utils/platform_utils.h"

#include "lines/linestorage.h"

#include <QDebug>

class StationsSQLModelResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::StationsModelResult);
    inline StationsSQLModelResultEvent() : QEvent(_Type) {}

    QVector<StationsSQLModel::StationItem> items;
    int firstRow;
};

//Error messages
static constexpr char
errorNameAlreadyUsedText[] = QT_TRANSLATE_NOOP("StationsSQLModel",
                                               "The name <b>%1</b> is already used by another station.<br>"
                                               "Please choose a different name for each station.");
static constexpr char
errorShortNameAlreadyUsedText[] = QT_TRANSLATE_NOOP("StationsSQLModel",
                                                    "The name <b>%1</b> is already used as short name for station <b>%2</b>.<br>"
                                                    "Please choose a different name for each station.");
static constexpr char
errorNameSameShortNameText[] = QT_TRANSLATE_NOOP("StationsSQLModel",
                                                 "Name and short name cannot be equal (<b>%1</b>).");

StationsSQLModel::StationsSQLModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(500, db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
    sortColumn = NameCol;
    LineStorage *lines = Session->mLineStorage;
    connect(lines, &LineStorage::stationAdded, this, &StationsSQLModel::onStationAdded);
    connect(lines, &LineStorage::stationRemoved, this, &StationsSQLModel::onStationRemoved);
}

bool StationsSQLModel::event(QEvent *e)
{
    if(e->type() == StationsSQLModelResultEvent::_Type)
    {
        StationsSQLModelResultEvent *ev = static_cast<StationsSQLModelResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

QVariant StationsSQLModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (section)
            {
            case NameCol:
                return tr("Name");
            case ShortName:
                return tr("Short Name");
            case Platforms:
                return tr("Platforms");
            case Depots:
                return tr("Depots");
            case PlatformColor:
                return tr("Main Color");
            case DefaultFreightPlatf:
                return tr("Freight Plaftorm");
            case DefaultPassengerPlatf:
                return tr("Passenger Platf");
            }
            break;
        }
        case Qt::ToolTipRole:
        {
            switch (section)
            {
            case DefaultFreightPlatf:
                return tr("Default platform for newly created stops when job category is Freight, Postal or Engine movement");
            case DefaultPassengerPlatf:
                return tr("Default platform for newly created stops when job is passenger train");
            }
            break;
        }
        }
    }
    else if(role == Qt::DisplayRole)
    {
        return section + curPage * ItemsPerPage + 1;
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

int StationsSQLModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int StationsSQLModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant StationsSQLModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<StationsSQLModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const StationItem& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case NameCol:
            return item.name;
        case ShortName:
            return item.shortName;
        case Platforms:
            return item.platfs;
        case Depots:
            return item.depots;
        case DefaultFreightPlatf:
            return utils::shortPlatformName(item.defaultFreightPlatf);
        case DefaultPassengerPlatf:
            return utils::shortPlatformName(item.defaultPassengerPlatf);
        }
        break;
    }

    case STATION_ID:
        return item.stationId;
    case COLOR_ROLE:
        return item.platfColor;
    case PLATF_ID:
    {
        switch (idx.column())
        {
        case DefaultFreightPlatf:
            return item.defaultFreightPlatf;
        case DefaultPassengerPlatf:
            return item.defaultPassengerPlatf;
        }
    }
    }

    return QVariant();
}

bool StationsSQLModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    StationItem &item = cache[row - cacheFirstRow];
    QModelIndex first = idx;
    QModelIndex last = idx;

    switch (role)
    {
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case NameCol:
        {
            if(!setName(item, value.toString()))
                return false;
            break;
        }
        case ShortName:
        {
            if(!setShortName(item, value.toString()))
                return false;
            break;
        }
        case Platforms:
        {
            bool ok = false;
            int val = value.toInt(&ok);
            if(!ok || !setPlatfCount(item, val))
                return false;
            break;
            break;
        }
        case Depots:
        {
            bool ok = false;
            int val = value.toInt(&ok);
            if(!ok || !setDepotCount(item, val))
                return false;
            break;
            break;
        }
        case DefaultFreightPlatf:
        {
            bool ok = false;
            int val = value.toInt(&ok);
            if(!ok || !setDefaultFreightPlatf(item, val))
                return false;
            break;
        }
        case DefaultPassengerPlatf:
        {
            bool ok = false;
            int val = value.toInt(&ok);
            if(!ok || !setDefaultPassengerPlatf(item, val))
                return false;
            break;
        }
        }
        break;
    }
    case COLOR_ROLE:
    {
        QRgb rgb = value.toUInt();
        if(item.platfColor == rgb)
            return false;

        command q_setPlatfColor(mDb, "UPDATE stations SET platf_color=? WHERE id=?");
        q_setPlatfColor.bind(1, qint64(rgb));
        q_setPlatfColor.bind(2, item.stationId);
        if(q_setPlatfColor.execute() != SQLITE_OK)
        {
            qDebug() << "Error setting color station:" << item.stationId << mDb.error_msg();
            return false;
        }
        item.platfColor = rgb;
        emit Session->mLineStorage->stationColorChanged(item.stationId);
    }
    }

    emit dataChanged(first, last);

    return true;
}

Qt::ItemFlags StationsSQLModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    f.setFlag(Qt::ItemIsEditable);

    return f;
}

void StationsSQLModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void StationsSQLModel::refreshData(bool forceUpdate)
{
    if(!mDb.db())
        return;

    emit itemsReady(-1, -1); //Notify we are about to refresh

    //TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM stations");
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

void StationsSQLModel::setSortingColumn(int col)
{
    if(sortColumn == col || (col != NameCol && col != PlatformColor))
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

bool StationsSQLModel::addStation(db_id *outStationId)
{
    return Session->mLineStorage->addStation(outStationId);
}

bool StationsSQLModel::removeStation(db_id stationId)
{
    return Session->mLineStorage->removeStation(stationId);
}

void StationsSQLModel::onStationAdded()
{
    refreshData(); //Recalc row count
    setSortingColumn(NameCol);
    switchToPage(0); //Reset to first page and so it is shown as first row
}

void StationsSQLModel::onStationRemoved()
{
    refreshData(); //Recalc row count
}

void StationsSQLModel::fetchRow(int row)
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

void StationsSQLModel::internalFetch(int first, int sortCol, int valRow, const QVariant &val)
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

    QByteArray sql = "SELECT id,name,short_name,platforms,depot_platf,platf_color,defplatf_freight,defplatf_passenger FROM stations";
    switch (sortCol)
    {
    case NameCol:
    {
        whereCol = "name"; //Order by 1 column, no where clause
        break;
    }
    case Platforms:
    {
        whereCol = "platforms,name";
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

    QVector<StationItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    QRgb whiteColor = qRgb(255, 255, 255);

    if(reverse)
    {
        int i = BatchSize - 1;

        for(; it != end; ++it)
        {
            auto r = *it;
            StationItem &item = vec[i];
            item.stationId = r.get<db_id>(0);
            item.name = r.get<QString>(1);
            item.shortName = r.get<QString>(2);
            item.platfs = r.get<int>(3);
            item.depots = r.get<int>(4);
            if(r.column_type(5) == SQLITE_NULL)
                item.platfColor = whiteColor;
            else
                item.platfColor = QRgb(r.get<int>(5));
            item.defaultFreightPlatf = r.get<int>(6);
            item.defaultPassengerPlatf = r.get<int>(7);
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
            StationItem &item = vec[i];
            item.stationId = r.get<db_id>(0);
            item.name = r.get<QString>(1);
            item.shortName = r.get<QString>(2);
            item.platfs = r.get<int>(3);
            item.depots = r.get<int>(4);
            if(r.column_type(5) == SQLITE_NULL)
                item.platfColor = whiteColor;
            else
                item.platfColor = QRgb(r.get<int>(5));
            item.defaultFreightPlatf = r.get<int>(6);
            item.defaultPassengerPlatf = r.get<int>(7);
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    StationsSQLModelResultEvent *ev = new StationsSQLModelResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void StationsSQLModel::handleResult(const QVector<StationsSQLModel::StationItem> &items, int firstRow)
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
            QVector<StationItem> tmp = items;
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

bool StationsSQLModel::setName(StationsSQLModel::StationItem &item, const QString &val)
{
    const QString name = val.simplified();
    if(name.isEmpty() || item.name == name)
        return false;

    //TODO: check non allowed characters

    query q(mDb, "SELECT id,name FROM stations WHERE short_name=?");
    q.bind(1, name);
    if(q.step() == SQLITE_ROW)
    {
        db_id stId = q.getRows().get<db_id>(0);
        if(stId == item.stationId)
        {
            emit modelError(tr(errorNameSameShortNameText).arg(name));
        }
        else
        {
            const QString otherShortName = q.getRows().get<QString>(1);
            emit modelError(tr(errorShortNameAlreadyUsedText).arg(name, otherShortName));
        }
        return false;
    }

    q.prepare("UPDATE stations SET name=? WHERE id=?");
    q.bind(1, name);
    q.bind(2, item.stationId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        if(ret == SQLITE_CONSTRAINT_UNIQUE)
        {
            emit modelError(tr(errorNameAlreadyUsedText).arg(name));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }
        return false;
    }

    item.name = name;

    //This row has now changed position so we need to invalidate cache
    //HACK: we emit dataChanged for this index (that doesn't exist anymore)
    //but the view will trigger fetching at same scroll position so it is enough
    cache.clear();
    cacheFirstRow = 0;

    emit Session->mLineStorage->stationNameChanged(item.stationId);

    return true;
}

bool StationsSQLModel::setShortName(StationsSQLModel::StationItem &item, const QString &val)
{
    const QString shortName = val.simplified();
    if(item.name == shortName)
        return false;

    //TODO: check non allowed characters

    query q(mDb, "SELECT id,name FROM stations WHERE name=?");
    q.bind(1, shortName);
    if(q.step() == SQLITE_ROW)
    {
        db_id stId = q.getRows().get<db_id>(0);
        if(stId == item.stationId)
        {
            emit modelError(tr(errorNameSameShortNameText).arg(shortName));
        }
        else
        {
            const QString otherName = q.getRows().get<QString>(1);
            emit modelError(tr(errorShortNameAlreadyUsedText).arg(shortName, otherName));
        }
        return false;
    }

    q.prepare("UPDATE stations SET short_name=? WHERE id=?");
    if(shortName.isEmpty())
        q.bind(1); //Bind NULL
    else
        q.bind(1, shortName);
    q.bind(2, item.stationId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        if(ret == SQLITE_CONSTRAINT_UNIQUE)
        {
            emit modelError(tr(errorShortNameAlreadyUsedText).arg(shortName).arg(QString()));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }
        return false;
    }

    item.shortName = shortName;

    emit Session->mLineStorage->stationNameChanged(item.stationId);

    return true;
}

bool StationsSQLModel::setPlatfCount(StationsSQLModel::StationItem &item, int platf)
{
    if(item.platfs == platf || platf < 0 || (platf == 0 && item.depots == 0)) //If there are no depots there must be at least 1 main platform
        return false;
    item.platfs = qint8(platf);

    command q_setPlatf(mDb, "UPDATE stations SET platforms=? WHERE id=?");
    q_setPlatf.bind(1, item.platfs);
    q_setPlatf.bind(2, item.stationId);
    q_setPlatf.execute();
    q_setPlatf.reset();

    //If ob.platfs == 0 -> they are set to -1 which is first depot platf
    if(item.defaultFreightPlatf >= item.platfs)
        setDefaultFreightPlatf(item, item.platfs - 1);
    if(item.defaultPassengerPlatf >= item.platfs)
        setDefaultPassengerPlatf(item, item.platfs - 1);

    emit Session->mLineStorage->stationModified(item.stationId);

    return true;
}

bool StationsSQLModel::setDepotCount(StationsSQLModel::StationItem &item, int depots)
{
    if(item.depots == depots || depots < 0 || (depots == 0 && item.platfs == 0)) //If there are no depots there must be at least 1 main platform
        return false;
    item.depots = qint8(depots);

    command q_setPlatf(mDb, "UPDATE stations SET depot_platf=? WHERE id=?");
    q_setPlatf.bind(1, item.depots);
    q_setPlatf.bind(2, item.stationId);
    q_setPlatf.execute();
    q_setPlatf.reset();

    //If ob.platfs == 0 -> they are set to -1 which is first depot platf
    if(item.defaultFreightPlatf < -item.depots)
        setDefaultFreightPlatf(item,  -item.depots);
    if(item.defaultPassengerPlatf < -item.depots)
        setDefaultPassengerPlatf(item, -item.depots);

    emit Session->mLineStorage->stationModified(item.stationId);

    return true;
}

bool StationsSQLModel::setDefaultFreightPlatf(StationsSQLModel::StationItem &item, int platf)
{
    if(platf >= item.platfs || platf < -item.depots)
        return false;

    command cmd(mDb, "UPDATE stations SET defplatf_freight=? WHERE id=?");
    cmd.bind(1, platf);
    cmd.bind(2, item.stationId);
    if(cmd.execute() != SQLITE_OK)
        return false;
    item.defaultFreightPlatf = qint8(platf);
    return true;
}

bool StationsSQLModel::setDefaultPassengerPlatf(StationsSQLModel::StationItem &item, int platf)
{
    if(platf >= item.platfs || platf < -item.depots)
        return false;

    command cmd(mDb, "UPDATE stations SET defplatf_passenger=? WHERE id=?");
    cmd.bind(1, platf);
    cmd.bind(2, item.stationId);
    if(cmd.execute() != SQLITE_OK)
        return false;
    item.defaultPassengerPlatf = qint8(platf);
    return true;
}
