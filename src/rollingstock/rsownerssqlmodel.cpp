#include "rsownerssqlmodel.h"

#include <QCoreApplication>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/rs_types_names.h"

#include "utils/worker_event_types.h"

#include <QDebug>

class RSOwnersResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::RsOwnersModelResult);

    inline RSOwnersResultEvent() : QEvent(_Type) {}

    QVector<RSOwnersSQLModel::RSOwner> items;
    int firstRow;
};

static constexpr char
errorOwnerNameAlreadyUsed[] = QT_TRANSLATE_NOOP("RSOwnersSQLModel",
                                                "This owner name (<b>%1</b>) is already used.");

static constexpr char
errorOwnerInUseCannotDelete[] = QT_TRANSLATE_NOOP("RSOwnersSQLModel",
                                                  "There are rollingstock pieces of owner <b>%1</b> so it cannot be removed.\n"
                                                  "If you wish to remove it, please first delete all <b>%1</b> pieces.");

RSOwnersSQLModel::RSOwnersSQLModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(500, db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
    sortColumn = Name;
}

bool RSOwnersSQLModel::event(QEvent *e)
{
    if(e->type() == RSOwnersResultEvent::_Type)
    {
        RSOwnersResultEvent *ev = static_cast<RSOwnersResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

QVariant RSOwnersSQLModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case Name:
                return RsTypeNames::tr("Name");
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

int RSOwnersSQLModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int RSOwnersSQLModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant RSOwnersSQLModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    //qDebug() << "Data:" << idx.row();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<RSOwnersSQLModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const RSOwner& item = cache.at(row - cacheFirstRow);

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

bool RSOwnersSQLModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    RSOwner &item = cache[row - cacheFirstRow];
    QModelIndex first = idx;
    QModelIndex last = idx;

    switch (role)
    {
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case Name:
        {
            QString newName = value.toString().simplified();
            if(item.name == newName)
                return false; //No change

            command set_name(mDb, "UPDATE rs_owners SET name=? WHERE id=?");

            if(newName.isEmpty())
                set_name.bind(1); //Bind NULL
            else
                set_name.bind(1, newName);
            set_name.bind(2, item.ownerId);
            int ret = set_name.execute();
            if(ret == SQLITE_CONSTRAINT_UNIQUE)
            {
                emit modelError(tr(errorOwnerNameAlreadyUsed).arg(newName));
                return false;
            }
            else if(ret != SQLITE_OK)
            {
                return false;
            }

            item.name = newName;
            //FIXME: maybe emit some signals?

            //This row has now changed position so we need to invalidate cache
            //HACK: we emit dataChanged for this index (that doesn't exist anymore)
            //but the view will trigger fetching at same scroll position so it is enough
            cache.clear();
            cacheFirstRow = 0;

            break;
        }
        default:
            return false;
        }
        break;
    }
    default:
        return false;
    }

    emit dataChanged(first, last);
    return true;
}

Qt::ItemFlags RSOwnersSQLModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet
    f.setFlag(Qt::ItemIsEditable);
    return f;
}

void RSOwnersSQLModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void RSOwnersSQLModel::refreshData()
{
    if(!mDb.db())
        return;

    //TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM rs_owners");
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

void RSOwnersSQLModel::fetchRow(int row)
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

void RSOwnersSQLModel::internalFetch(int first, int sortCol, int valRow, const QVariant& val)
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

    const char *whereCol;

    QByteArray sql = "SELECT id,name FROM rs_owners";
    switch (sortCol)
    {
    case Name:
    {
        whereCol = "name"; //Order by 2 columns, no where clause
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
        case Name:
        {
            q.bind(3, val.toString());
            break;
        }
        }
    }

    QVector<RSOwner> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    if(reverse)
    {
        int i = BatchSize - 1;

        for(; it != end; ++it)
        {
            auto r = *it;
            RSOwner &item = vec[i];
            item.ownerId = r.get<db_id>(0);
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
            RSOwner &item = vec[i];
            item.ownerId = r.get<db_id>(0);
            item.name = r.get<QString>(1);
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    RSOwnersResultEvent *ev = new RSOwnersResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void RSOwnersSQLModel::handleResult(const QVector<RSOwner>& items, int firstRow)
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
            QVector<RSOwner> tmp = items;
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

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

void RSOwnersSQLModel::setSortingColumn(int /*col*/)
{
    //Only sort by name
}

bool RSOwnersSQLModel::removeRSOwner(db_id ownerId, const QString& name)
{
    if(!ownerId)
        return false;

    command cmd(mDb, "DELETE FROM rs_owners WHERE id=?");
    cmd.bind(1, ownerId);
    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        ret = mDb.extended_error_code();
        if(ret == SQLITE_CONSTRAINT_TRIGGER)
        {
            QString tmp = name;
            if(name.isNull())
            {
                query q(mDb, "SELECT name FROM rs_owners WHERE id=?");
                q.bind(1, ownerId);
                q.step();
                tmp = q.getRows().get<QString>(0);
            }

            emit modelError(tr(errorOwnerInUseCannotDelete).arg(tmp));
            return false;
        }
        qWarning() << "RSOwnersSQLModel: error removing owner" << ret << mDb.error_msg();
        return false;
    }

    refreshData();
    return true;
}

bool RSOwnersSQLModel::removeRSOwnerAt(int row)
{
    if(row >= curItemCount || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    const RSOwner &item = cache.at(row - cacheFirstRow);

    return removeRSOwner(item.ownerId, item.name);
}

db_id RSOwnersSQLModel::addRSOwner(int *outRow)
{
    db_id ownerId = 0;

    command cmd(mDb, "INSERT INTO rs_owners(id,name) VALUES (NULL,'')");
    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret = cmd.execute();
    if(ret == SQLITE_OK)
    {
        ownerId = mDb.last_insert_rowid();
    }
    sqlite3_mutex_leave(mutex);

    if(ret == SQLITE_CONSTRAINT_UNIQUE)
    {
        //There is already an empty owner, use that instead
        query findEmpty(mDb, "SELECT id FROM rs_owners WHERE name='' OR name IS NULL LIMIT 1");
        if(findEmpty.step() == SQLITE_ROW)
        {
            ownerId = findEmpty.getRows().get<db_id>(0);
        }
    }
    else if(ret != SQLITE_OK)
    {
        qDebug() << "RS Owner Error adding:" << ret << mDb.error_msg() << mDb.error_code() << mDb.extended_error_code();
    }

    refreshData(); //Recalc row count
    switchToPage(0); //Reset to first page and so it is shown as first row

    if(outRow)
        *outRow = ownerId ? 0 : -1; //Empty name is always the first

    return ownerId;
}

bool RSOwnersSQLModel::removeAllRSOwners()
{
    command cmd(mDb, "DELETE FROM rs_owners");
    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        qWarning() << "Removing ALL RS OWNERS:" << ret << mDb.extended_error_code() << "Err:" << mDb.error_msg();
        return false;
    }

    refreshData();
    return true;
}
