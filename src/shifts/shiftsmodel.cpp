#include "shiftsmodel.h"

#include "utils/delegates/sql/pageditemmodelhelper_impl.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "shiftresultevent.h"

#include <QDebug>

ShiftsModel::ShiftsModel(database &db, QObject *parent) :
    BaseClass(500, db, parent)
{
    sortColumn = ShiftName;
}

QVariant ShiftsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case ShiftName:
                return tr("Shift Name");
            }
        }
        else
        {
            return section + curPage * ItemsPerPage + 1;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
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
    QByteArray sql = "SELECT COUNT(1) FROM jobshifts";

    if(!m_nameFilter.isEmpty())
    {
        sql.append(" WHERE jobshifts.name LIKE ?1");
    }

    query q(mDb, sql);

    QByteArray nameFilter;
    if(!m_nameFilter.isEmpty())
    {
        nameFilter.reserve(m_nameFilter.size() + 2);
        nameFilter.append('%');
        nameFilter.append(m_nameFilter.toUtf8());
        nameFilter.append('%');

        sqlite3_bind_text(q.stmt(), 1, nameFilter, nameFilter.size(), SQLITE_STATIC);
    }

    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

std::pair<QString, IPagedItemModel::FilterFlags> ShiftsModel::getFilterAtCol(int col)
{
    switch (col)
    {
    case ShiftName:
        return {m_nameFilter, FilterFlag::BasicFiltering};
    }

    return {QString(), FilterFlag::NoFiltering};
}

bool ShiftsModel::setFilterAtCol(int col, const QString &str)
{
    const bool isNull = str.startsWith(nullFilterStr, Qt::CaseInsensitive);

    switch (col)
    {
    case ShiftName:
    {
        if(isNull)
            return false; //Cannot have NULL Name
        m_nameFilter = str;
        break;
    }
    default:
        return false;
    }

    emit filterChanged();
    return true;
}

void ShiftsModel::internalFetch(int first, int /*sortColumn*/, int /*valRow*/, const QVariant &/*val*/)
{
    query q(mDb);

    int offset = first;

    qDebug() << "Fetching:" << first << "Offset:" << offset;

    QByteArray sql = "SELECT id,name FROM jobshifts";

    if(!m_nameFilter.isEmpty())
    {
        sql.append(" WHERE name LIKE ?3");
    }

    sql += " ORDER BY name LIMIT ?1";

    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);

    QByteArray nameFilter;
    if(!m_nameFilter.isEmpty())
    {
        nameFilter.reserve(m_nameFilter.size() + 2);
        nameFilter.append('%');
        nameFilter.append(m_nameFilter.toUtf8());
        nameFilter.append('%');

        sqlite3_bind_text(q.stmt(), 3, nameFilter, nameFilter.size(), SQLITE_STATIC);
    }

    QVector<ShiftItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

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

    postResult(vec, first);
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
    m_nameFilter.clear();
    m_nameFilter.squeeze();
    emit filterChanged();

    refreshData(); //Recalc row count

    if(outRow)
        *outRow = shiftId ? 0 : -1; //Empty name is always the first

    if(shiftId)
        emit Session->shiftAdded(shiftId);

    return shiftId;
}
