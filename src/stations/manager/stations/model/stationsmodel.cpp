#include "stationsmodel.h"

#include <QCoreApplication>
#include <QEvent>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/worker_event_types.h"

#include "stations/station_name_utils.h"

#include "app/session.h"

#include <QDebug>

class StationsSQLModelResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::StationsModelResult);
    inline StationsSQLModelResultEvent() : QEvent(_Type) {}

    QVector<StationsModel::StationItem> items;
    int firstRow;
};

//Error messages
static constexpr char
    errorNameAlreadyUsedText[] = QT_TRANSLATE_NOOP("StationsModel",
                        "The name <b>%1</b> is already used by another station.<br>"
                        "Please choose a different name for each station.");
static constexpr char
    errorShortNameAlreadyUsedText[] = QT_TRANSLATE_NOOP("StationsModel",
                        "The name <b>%1</b> is already used as short name for station <b>%2</b>.<br>"
                        "Please choose a different name for each station.");
static constexpr char
    errorNameSameShortNameText[] = QT_TRANSLATE_NOOP("StationsModel",
                        "Name and short name cannot be equal (<b>%1</b>).");

static constexpr char
    errorPhoneSameNumberText[] = QT_TRANSLATE_NOOP("StationsModel",
                        "The phone number <b>%1</b> is already used by another station.<br>"
                        "Please choose a different phone number for each station.");

static constexpr char
    errorStationInUseText[] = QT_TRANSLATE_NOOP("StationsModel",
                        "Cannot delete <b>%1</b> station because it is stille referenced.<br>"
                        "Please delete all jobs stopping here and remove the station from any line.");

StationsModel::StationsModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(500, db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
    sortColumn = NameCol;
}

bool StationsModel::event(QEvent *e)
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

QVariant StationsModel::headerData(int section, Qt::Orientation orientation, int role) const
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
            case ShortNameCol:
                return tr("Short Name");
            case TypeCol:
                return tr("Type");
            case PhoneCol:
                return tr("Phone");
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

int StationsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int StationsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant StationsModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<StationsModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const StationItem& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case NameCol:
            return item.name;
        case ShortNameCol:
            return item.shortName;
        case TypeCol:
            return utils::StationUtils::name(item.type);
        case PhoneCol:
        {
            if(item.phone_number == -1)
                return QVariant(); //Null
            return item.phone_number;
        }
        }
        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case NameCol:
            return item.name;
        case ShortNameCol:
            return item.shortName;
        case TypeCol:
            return int(item.type);
        case PhoneCol:
            return item.phone_number;
        }
        break;
    }
    }

    return QVariant();
}

bool StationsModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size() || role != Qt::EditRole)
        return false; //Not fetched yet or invalid

    StationItem &item = cache[row - cacheFirstRow];
    QModelIndex first = idx;
    QModelIndex last = idx;

    switch (idx.column())
    {
    case NameCol:
    {
        if(!setName(item, value.toString()))
            return false;
        break;
    }
    case ShortNameCol:
    {
        if(!setShortName(item, value.toString()))
            return false;
        break;
    }
    case TypeCol:
    {
        bool ok = false;
        int val = value.toInt(&ok);
        if(!ok || !setType(item, val))
            return false;
        break;
    }
    case PhoneCol:
    {
        bool ok = false;
        qint64 val = value.toLongLong(&ok);
        if(!ok)
            val = -1;
        if(!setPhoneNumber(item, val))
            return false;
        break;
    }
    }

    emit dataChanged(first, last);

    return true;
}

Qt::ItemFlags StationsModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    f.setFlag(Qt::ItemIsEditable);

    return f;
}

qint64 StationsModel::recalcTotalItemCount()
{
    //TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM stations");
    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void StationsModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void StationsModel::setSortingColumn(int col)
{
    if(sortColumn == col || (col != NameCol && col != TypeCol))
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

bool StationsModel::addStation(const QString &name, db_id *outStationId)
{
    if(name.isEmpty())
        return false;

    command q_newStation(mDb, "INSERT INTO stations(id,name,short_name,type,phone_number,svg_data)"
                              " VALUES (NULL, ?, NULL, 0, NULL, NULL)");
    q_newStation.bind(1, name);

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret = q_newStation.execute();
    db_id stationId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_newStation.reset();

    if((ret != SQLITE_OK && ret != SQLITE_DONE) || stationId == 0)
    {
        //Error
        if(outStationId)
            *outStationId = 0;

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

    if(outStationId)
        *outStationId = stationId;

    refreshData(); //Recalc row count
    setSortingColumn(NameCol);
    switchToPage(0); //Reset to first page and so it is shown as first row

    return true;
}

bool StationsModel::removeStation(db_id stationId)
{
    command q_removeStation(mDb, "DELETE FROM stations WHERE id=?");

    q_removeStation.bind(1, stationId);
    int ret = q_removeStation.execute();
    q_removeStation.reset();

    if(ret != SQLITE_OK)
    {
        if(ret == SQLITE_CONSTRAINT_TRIGGER)
        {
            //TODO: show more information to the user, like where it's still referenced
            query q(mDb, "SELECT name FROM stations WHERE id=?");
            q.bind(1, stationId);
            if(q.step() == SQLITE_ROW)
            {
                const QString name = q.getRows().get<QString>(0);
                emit modelError(tr(errorStationInUseText).arg(name));
            }
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }

        return false;
    }

    emit Session->stationRemoved(stationId);

    refreshData(); //Recalc row count

    return true;
}

void StationsModel::fetchRow(int row)
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

void StationsModel::internalFetch(int first, int sortCol, int valRow, const QVariant &val)
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

    QByteArray sql = "SELECT id,name,short_name,type,phone_number FROM stations";
    switch (sortCol)
    {
    case NameCol:
    {
        whereCol = "name"; //Order by 1 column, no where clause
        break;
    }
    case TypeCol:
    {
        whereCol = "type,name";
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
            item.type = utils::StationType(r.get<int>(3));
            if(r.column_type(4) == SQLITE_NULL)
                item.phone_number = -1;
            else
                item.phone_number = r.get<qint64>(4);
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
            item.type = utils::StationType(r.get<int>(3));
            if(r.column_type(4) == SQLITE_NULL)
                item.phone_number = -1;
            else
                item.phone_number = r.get<qint64>(4);
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

void StationsModel::handleResult(const QVector<StationsModel::StationItem> &items, int firstRow)
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

bool StationsModel::setName(StationsModel::StationItem &item, const QString &val)
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

    emit Session->stationNameChanged(item.stationId);

    //This row has now changed position so we need to invalidate cache
    //HACK: we emit dataChanged for this index (that doesn't exist anymore)
    //but the view will trigger fetching at same scroll position so it is enough
    cache.clear();
    cacheFirstRow = 0;

    return true;
}

bool StationsModel::setShortName(StationsModel::StationItem &item, const QString &val)
{
    const QString shortName = val.simplified();
    if(item.shortName == shortName)
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
    emit Session->stationNameChanged(item.stationId);

    return true;
}

bool StationsModel::setType(StationsModel::StationItem &item, int val)
{
    utils::StationType type = utils::StationType(val);
    if(val < 0 || type >= utils::StationType::NTypes || item.type == type)
        return false;

    query q(mDb, "UPDATE stations SET type=? WHERE id=?");
    q.bind(1, val);
    q.bind(2, item.stationId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        return false;
    }

    item.type = type;

    if(sortColumn == TypeCol)
    {
        //This row has now changed position so we need to invalidate cache
        //HACK: we emit dataChanged for this index (that doesn't exist anymore)
        //but the view will trigger fetching at same scroll position so it is enough
        cache.clear();
        cacheFirstRow = 0;
    }

    return true;
}

bool StationsModel::setPhoneNumber(StationsModel::StationItem &item, qint64 val)
{
    if(item.phone_number == val)
        return false;

    //TODO: better handling of NULL (= remove phone number)
    //TODO: maybe is better TEXT type for phone number
    if(val < 0)
        val = -1;

    query q(mDb, "UPDATE stations SET phone_number=? WHERE id=?");
    if(val == -1)
        q.bind(1); //Bind NULL
    else
        q.bind(1, val);
    q.bind(2, item.stationId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        if(ret == SQLITE_CONSTRAINT_UNIQUE)
        {
            emit modelError(tr(errorPhoneSameNumberText).arg(val));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }
        return false;
    }

    item.phone_number = val;

    return true;
}
