#include "importstationmodel.h"

#include "utils/sqldelegate/pageditemmodelhelper_impl.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "stations/station_name_utils.h"

#include <QDebug>

ImportStationModel::ImportStationModel(sqlite3pp::database &db, QObject *parent) :
    BaseClass(500, db, parent)
{
    sortColumn = NameCol;
}

QVariant ImportStationModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant ImportStationModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<ImportStationModel *>(this)->fetchRow(row);

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
        }
        break;
    }
    }

    return QVariant();
}

qint64 ImportStationModel::recalcTotalItemCount()
{
    QByteArray sql = "SELECT COUNT(id) FROM stations";
    if(!mNameFilter.isEmpty())
    {
        sql += " WHERE name LIKE ?1 OR short_name LIKE ?1";
    }

    query q(mDb, sql);
    if(!mNameFilter.isEmpty())
        sqlite3_bind_text(q.stmt(), 1, mNameFilter, mNameFilter.size(), SQLITE_STATIC);
    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void ImportStationModel::setSortingColumn(int col)
{
    if(sortColumn == col || (col != NameCol && col != TypeCol))
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

void ImportStationModel::filterByName(const QString &text)
{
    mNameFilter.clear();
    if(!text.isEmpty())
    {
        mNameFilter.clear();
        mNameFilter.reserve(text.size() + 2);
        mNameFilter.append('%');
        mNameFilter.append(text.toUtf8());
        mNameFilter.append('%');
    }

    refreshData(true);
}

void ImportStationModel::internalFetch(int first, int sortCol, int valRow, const QVariant &val)
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

    QByteArray sql = "SELECT id,name,short_name,type FROM stations";
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

    if(!mNameFilter.isEmpty())
    {
        sql += " WHERE name LIKE ?3 OR short_name LIKE ?3";
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
    if(!mNameFilter.isEmpty())
    {
        sqlite3_bind_text(q.stmt(), 3, mNameFilter, mNameFilter.size(), SQLITE_STATIC);
    }

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

    int i = reverse ? BatchSize - 1 : 0;
    const int increment = reverse ? -1 : 1;

    for(; it != end; ++it)
    {
        auto r = *it;
        StationItem &item = vec[i];
        item.stationId = r.get<db_id>(0);
        item.name = r.get<QString>(1);
        item.shortName = r.get<QString>(2);
        item.type = utils::StationType(r.get<int>(3));

        i += increment;
    }

    if(reverse && i > -1)
        vec.remove(0, i + 1);
    else if(i < BatchSize)
        vec.remove(i, BatchSize - i);

    postResult(vec, first);
}
