#include "rsownerssqlmodel.h"

#include "utils/delegates/sql/pageditemmodelhelper_impl.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/rs_types_names.h"

#include <QDebug>

static constexpr char
errorOwnerNameAlreadyUsed[] = QT_TRANSLATE_NOOP("RSOwnersSQLModel",
                                                "This owner name (<b>%1</b>) is already used.");

static constexpr char
errorOwnerInUseCannotDelete[] = QT_TRANSLATE_NOOP("RSOwnersSQLModel",
                                                  "There are rollingstock pieces of owner <b>%1</b> so it cannot be removed.\n"
                                                  "If you wish to remove it, please first delete all <b>%1</b> pieces.");

RSOwnersSQLModel::RSOwnersSQLModel(sqlite3pp::database &db, QObject *parent) :
    BaseClass(500, db, parent)
{
    sortColumn = Name;
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

std::pair<QString, IPagedItemModel::FilterFlags> RSOwnersSQLModel::getFilterAtCol(int col)
{
    switch (col)
    {
    case Name:
        return {m_ownerFilter, FilterFlag::BasicFiltering};
    }

    return {QString(), FilterFlag::NoFiltering};
}

bool RSOwnersSQLModel::setFilterAtCol(int col, const QString &str)
{
    switch (col)
    {
    case Name:
    {
        if(str.startsWith(nullFilterStr, Qt::CaseInsensitive))
            return false; //Cannot have NULL OwnerName
        m_ownerFilter = str;
        emit filterChanged();
        return true;
    }
    }

    return false;
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

    refreshData(); //Recalc row count
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

    //Clear filters
    m_ownerFilter.clear();
    m_ownerFilter.squeeze();
    emit filterChanged();

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

    refreshData(); //Recalc row count
    return true;
}

qint64 RSOwnersSQLModel::recalcTotalItemCount()
{
    QByteArray sql = "SELECT COUNT(1) FROM rs_owners";
    if(!m_ownerFilter.isEmpty())
    {
        sql.append(" WHERE rs_owners.name LIKE ?1");
    }

    query q(mDb, sql);

    if(!m_ownerFilter.isEmpty())
    {
        QByteArray ownerFilter;
        ownerFilter.reserve(m_ownerFilter.size() + 2);
        ownerFilter.append('%');
        ownerFilter.append(m_ownerFilter.toUtf8());
        ownerFilter.append('%');
        q.bind(1, ownerFilter, sqlite3pp::copy);
    }

    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void RSOwnersSQLModel::internalFetch(int first, int sortCol, int /*valRow*/, const QVariant& /*val*/)
{
    query q(mDb);

    int offset = first + curPage * ItemsPerPage;

    qDebug() << "Fetching:" << first << "Offset:" << offset;


    QByteArray sql = "SELECT id,name FROM rs_owners";
    if(!m_ownerFilter.isEmpty())
    {
        sql.append(" WHERE rs_owners.name LIKE ?3");
    }

    const char *sertColExpr = nullptr;
    switch (sortCol)
    {
    case Name:
    {
        sertColExpr = "name"; //Order by 2 columns, no where clause
        break;
    }
    }

    sql += " ORDER BY ";
    sql += sertColExpr;

    sql += " LIMIT ?1";
    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);

    if(!m_ownerFilter.isEmpty())
    {
        QByteArray ownerFilter;
        ownerFilter.reserve(m_ownerFilter.size() + 2);
        ownerFilter.append('%');
        ownerFilter.append(m_ownerFilter.toUtf8());
        ownerFilter.append('%');
        q.bind(3, ownerFilter, sqlite3pp::copy);
    }

    QVector<RSOwner> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    int i = 0;
    for(; it != end; ++it)
    {
        auto r = *it;
        RSOwner &item = vec[i];
        item.ownerId = r.get<db_id>(0);
        item.name = r.get<QString>(1);

        i += 1;
    }

    if(i < BatchSize)
        vec.remove(i, BatchSize - i);

    postResult(vec, first);
}
