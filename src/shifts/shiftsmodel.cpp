#include "shiftsmodel.h"

#include "app/session.h"

#include <QDebug>

#include <QCoreApplication>
#include "shiftresultevent.h"

ShiftsModel::ShiftsModel(database &db, QObject *parent) :
    IPagedItemModel(500, db, parent),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize)
{
}

bool ShiftsModel::event(QEvent *e)
{
    if(e->type() == ShiftResultEvent::_Type)
    {
        ShiftResultEvent *ev = static_cast<ShiftResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

QVariant ShiftsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case ShiftName:
            return tr("Shift Name");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int ShiftsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int ShiftsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant ShiftsModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cach
        const_cast<ShiftsModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        const ShiftItem& item = cache.at(idx.row() - cacheFirstRow);

        switch (idx.column())
        {
        case ShiftName:
            return item.shiftName;
        }
    }
    }

    return QVariant();
}

bool ShiftsModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    ShiftItem &item = cache[row - cacheFirstRow];

    switch (role)
    {
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case ShiftName:
        {
            QString newName = value.toString().simplified();
            if(item.shiftName == newName)
                return false; //No change

            command set_name(mDb, "UPDATE jobshifts SET name=? WHERE id=?");

            if(newName.isEmpty())
                set_name.bind(1); //Bind NULL
            else
                set_name.bind(1, newName);
            set_name.bind(2, item.shiftId);
            int ret = set_name.execute();
            if(ret == SQLITE_CONSTRAINT_UNIQUE)
            {
                emit modelError(tr("There is already another job shift with same name: <b>%1</b>").arg(newName));
                return false;
            }
            else if(ret != SQLITE_OK)
            {
                return false;
            }

            item.shiftName = newName;
            emit Session->shiftNameChanged(item.shiftId);

            //This row has now changed position so we need to invalidate cache
            //HACK: we emit dataChanged for this index (that doesn't exist anymore)
            //but the view will trigger fetching at same scroll position so it is enough
            cache.clear();
            cacheFirstRow = 0;

            break;
        }
        }
    }
    }

    emit dataChanged(idx, idx);
    return true;
}

Qt::ItemFlags ShiftsModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f;

    return f | Qt::ItemIsEditable;
}

qint64 ShiftsModel::recalcTotalItemCount()
{
    query q(mDb);
    if(mQuery.size() <= 2) // '%%' -> '%<name>%'
    {
        q.prepare("SELECT COUNT(1) FROM jobshifts");
    }else{
        q.prepare("SELECT COUNT(1) FROM jobshifts WHERE name LIKE ?");
        q.bind(1, mQuery);
    }
    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void ShiftsModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void ShiftsModel::fetchRow(int row)
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

    QString val;
    int valRow = 0;
    ShiftItem *item = nullptr;

    if(cache.size())
    {
        if(firstPendingRow >= cacheFirstRow + cache.size())
        {
            valRow = cacheFirstRow + cache.size();
            item = &cache.last();
        }
        else if(firstPendingRow > (cacheFirstRow - firstPendingRow))
        {
            valRow = cacheFirstRow;
            item = &cache.first();
        }
    }

    if(item)
        val = item->shiftName;

    //TODO: use a custom QRunnable
    /*
    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection,
                              Q_ARG(int, firstPendingRow),
                              Q_ARG(int, valRow), Q_ARG(QString, val));
                              */
    internalFetch(firstPendingRow, valRow, val);
}

void ShiftsModel::internalFetch(int first, int valRow, const QString& val)
{
    query q(mDb);

    int offset = first - valRow;
    bool reverse = false;

    if(valRow > first)
    {
        offset = 0;
        reverse = true;
    }

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    QByteArray sql = "SELECT id,name FROM jobshifts";
    if(!val.isEmpty())
    {
        sql += " WHERE name";
        if(reverse)
            sql += "<?3";
        else
            sql += ">?3";
    }

    if(!mQuery.isEmpty())
    {
        if(val.isEmpty())
            sql += " WHERE name LIKE ?4";
        else
            sql += " AND name LIKE ?4";
    }

    sql += " ORDER BY name";

    if(reverse)
        sql += " DESC";

    sql += " LIMIT ?1";
    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);

    if(!val.isEmpty())
        q.bind(3, val);

    if(!mQuery.isEmpty())
        q.bind(4, mQuery);

    QVector<ShiftItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    if(reverse)
    {
        int i = BatchSize - 1;

        for(; it != end; ++it)
        {
            auto r = *it;
            ShiftItem &item = vec[i];
            item.shiftId = r.get<db_id>(0);
            item.shiftName = r.get<QString>(1);
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
            ShiftItem &item = vec[i];
            item.shiftId = r.get<db_id>(0);
            item.shiftName = r.get<QString>(1);
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    ShiftResultEvent *ev = new ShiftResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void ShiftsModel::handleResult(const QVector<ShiftItem> items, int firstRow)
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
            QVector<ShiftItem> tmp = items;
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

    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(firstRow + items.count() - 1, NCols - 1);
    emit dataChanged(firstIdx, lastIdx);

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

db_id ShiftsModel::shiftAtRow(int row) const
{
    if(row >= curItemCount || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return 0; //Not fetched yet or invalid

    return cache.at(row - cacheFirstRow).shiftId;
}

QString ShiftsModel::shiftNameAtRow(int row) const
{
    if(row >= curItemCount || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return QString(); //Not fetched yet or invalid

    return cache.at(row - cacheFirstRow).shiftName;
}

void ShiftsModel::setQuery(const QString &text)
{
    QString tmp = '%' + text + '%';
    if(mQuery == tmp)
        return;
    mQuery = tmp;

    refreshData(true);
}

bool ShiftsModel::removeShift(db_id shiftId)
{
    if(!shiftId)
        return false;

    command cmd(mDb, "DELETE FROM jobshifts WHERE id=?");
    cmd.bind(1, shiftId);
    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        qWarning() << "ShiftModel: error removing shift" << ret << mDb.error_msg();
        return false;
    }

    emit Session->shiftRemoved(shiftId);

    refreshData(); //Recalc row count
    return true;
}

bool ShiftsModel::removeShiftAt(int row)
{
    if(row >= curItemCount || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    return removeShift(cache.at(row - cacheFirstRow).shiftId);
}

db_id ShiftsModel::addShift(int *outRow)
{
    db_id shiftId = 0;

    command cmd(mDb, "INSERT INTO jobshifts(id, name) VALUES(NULL, '')");
    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret = cmd.execute();
    if(ret == SQLITE_OK)
    {
        shiftId = mDb.last_insert_rowid();
    }
    sqlite3_mutex_leave(mutex);

    if(ret == SQLITE_CONSTRAINT_UNIQUE)
    {
        //There is already an empty shift, use that instead
        query findEmpty(mDb, "SELECT id FROM jobshifts WHERE name = '' OR name IS NULL LIMIT 1");
        if(findEmpty.step() == SQLITE_ROW)
        {
            shiftId = findEmpty.getRows().get<db_id>(0);
        }
    }
    else if(ret != SQLITE_OK)
    {
        qDebug() << "Shift Error adding:" << ret;
    }

    //Reset filter
    mQuery.clear();
    mQuery.squeeze();

    refreshData(); //Recalc row count

    if(outRow)
        *outRow = shiftId ? 0 : -1; //Empty name is always the first

    if(shiftId)
        emit Session->shiftAdded(shiftId);

    return shiftId;
}
