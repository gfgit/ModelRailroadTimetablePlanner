/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "rollingstocksqlmodel.h"

#include "utils/delegates/sql/pageditemmodelhelper_impl.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/model_roles.h"
#include "utils/rs_types_names.h"
#include "utils/rs_utils.h"

#include <QDebug>

//ERRORMSG: show other errors
static constexpr char
    errorRSInUseCannotDelete[] = QT_TRANSLATE_NOOP("RollingstockSQLModel",
                        "Rollingstock item <b>%1</b> is used in some jobs so it cannot be removed.<br>"
                        "If you wish to remove it, please first remove it from its jobs.");

RollingstockSQLModel::RollingstockSQLModel(sqlite3pp::database &db, QObject *parent) :
    BaseClass(500, db, parent)
{
    sortColumn = Model;
}

QVariant RollingstockSQLModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role == Qt::DisplayRole)
    {
        if(orientation == Qt::Horizontal)
        {
            switch (section)
            {
            case Model:
                return RsTypeNames::tr("Model");
            case Number:
                return RsTypeNames::tr("Number");
            case Suffix:
                return RsTypeNames::tr("Suffix");
            case Owner:
                return RsTypeNames::tr("Owner");
            case TypeCol:
                return RsTypeNames::tr("Type");
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

QVariant RollingstockSQLModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    //qDebug() << "Data:" << idx.row();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<RollingstockSQLModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const RSItem& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case Model:
            return item.modelName;
        case Number:
            return rs_utils::formatNum(item.type, item.number);
        case Suffix:
            return item.modelSuffix;
        case Owner:
            return item.ownerName;
        case TypeCol:
            return RsTypeNames::name(item.type);
        }

        break;
    }
    case Qt::TextAlignmentRole:
    {
        if(idx.column() == Number)
        {
            return Qt::AlignVCenter + Qt::AlignRight;
        }
        break;
    }
    case RS_NUMBER:
        return item.number;
    case RS_IS_ENGINE:
        return item.type == RsType::Engine;
    }

    return QVariant();
}

bool RollingstockSQLModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    RSItem &item = cache[row - cacheFirstRow];
    QModelIndex first = idx;
    QModelIndex last = idx;

    if(role == RS_NUMBER)
    {
        bool ok = false;
        int number = value.toInt(&ok);
        if(!ok || !setNumber(item, number))
            return false;
    }

    emit dataChanged(first, last);
    return true;
}

Qt::ItemFlags RollingstockSQLModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    if(idx.column() != Suffix && idx.column() != TypeCol)
        f.setFlag(Qt::ItemIsEditable); //Suffix and TypeCol are deduced from Model

    return f;
}

void RollingstockSQLModel::setSortingColumn(int col)
{
    if(sortColumn == col || col == Number || col == Suffix || col >= NCols)
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

std::pair<QString, IPagedItemModel::FilterFlags> RollingstockSQLModel::getFilterAtCol(int col)
{
    switch (col)
    {
    case Model:
        return {m_modelFilter, FilterFlags(FilterFlag::BasicFiltering | FilterFlag::ExplicitNULL)};
    case Number:
        return {m_numberFilter, FilterFlag::BasicFiltering};
    case Owner:
        return {m_ownerFilter, FilterFlags(FilterFlag::BasicFiltering | FilterFlag::ExplicitNULL)};
    }

    return {QString(), FilterFlag::NoFiltering};
}

bool RollingstockSQLModel::setFilterAtCol(int col, const QString &str)
{
    const bool isNull = str.startsWith(nullFilterStr, Qt::CaseInsensitive);

    switch (col)
    {
    case Model:
    {
        m_modelFilter = str;
        break;
    }
    case Number:
    {
        if(isNull)
            return false; //Cannot have NULL Number
        m_numberFilter = str;
        break;
    }
    case Owner:
    {
        m_ownerFilter = str;
        break;
    }
    default:
        return false;
    }

    emit filterChanged();
    return true;
}

bool RollingstockSQLModel::getFieldData(int row, int col, db_id &idOut, QString &nameOut) const
{
    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false;

    const RSItem& item = cache[row - cacheFirstRow];

    switch (col)
    {
    case Model:
    {
        idOut = item.modelId;
        nameOut = item.modelName;
        break;
    }
    case Owner:
    {
        idOut = item.ownerId;
        nameOut = item.ownerName;
        break;
    }
    default:
        return false;
    }

    return true;
}

bool RollingstockSQLModel::validateData(int /*row*/, int /*col*/, db_id /*id*/, const QString &/*name*/)
{
    return true;
}

bool RollingstockSQLModel::setFieldData(int row, int col, db_id id, const QString &name)
{
    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false;

    RSItem& item = cache[row - cacheFirstRow];
    bool ret = false;
    switch (col)
    {
    case Model:
    {
        ret = setModel(item, id, name);
        break;
    }
    case Owner:
    {
        ret = setOwner(item, id, name);
        break;
    }
    default:
        break;
    }

    if(ret)
    {
        QModelIndex idx = index(row, col);
        emit dataChanged(idx, idx);
    }

    return ret;
}

bool RollingstockSQLModel::removeRSItem(db_id rsId, const RSItem *item)
{
    if(!rsId)
        return false;

    command cmd(mDb, "DELETE FROM rs_list WHERE id=?");
    cmd.bind(1, rsId);
    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        ret = mDb.extended_error_code();
        if(ret == SQLITE_CONSTRAINT_FOREIGNKEY || ret == SQLITE_CONSTRAINT_TRIGGER)
        {
            QString name;
            if(item)
            {
                name = rs_utils::formatName(item->modelName, item->number, item->modelSuffix, item->type);
            }
            else
            {
                query q(mDb, "SELECT rs_list.number,rs_models.name,rs_models.suffix,rs_models.type"
                             " FROM rs_list"
                             " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id"
                             " WHERE rs_list.id=?");
                q.bind(1, rsId);
                q.step();

                sqlite3_stmt *stmt = q.stmt();

                int number = sqlite3_column_int(stmt, 0);
                int modelNameLen = sqlite3_column_bytes(stmt, 1);
                const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 1));

                int modelSuffixLen = sqlite3_column_bytes(stmt, 2);
                const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 2));

                RsType type = RsType(sqlite3_column_int(stmt, 3));

                name = rs_utils::formatNameRef(modelName, modelNameLen,
                                               number,
                                               modelSuffix, modelSuffixLen,
                                               type);
            }

            emit modelError(tr(errorRSInUseCannotDelete).arg(name));
            return false;
        }
        qWarning() << "RollingstockSQLModel: error removing model" << ret << mDb.error_msg();
        return false;
    }

    emit Session->rollingstockRemoved(rsId);

    refreshData(); //Recalc row count
    return true;
}

bool RollingstockSQLModel::removeRSItemAt(int row)
{
    if(row >= curItemCount || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    const RSItem &item = cache.at(row - cacheFirstRow);
    return removeRSItem(item.rsId, &item);
}

db_id RollingstockSQLModel::addRSItem(int *outRow, QString *errOut)
{
    db_id rsId = 0;

    command cmd(mDb, "INSERT INTO rs_list(id,model_id,number,owner_id) VALUES (NULL,NULL,0,NULL)");
    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret = cmd.execute();
    if(ret == SQLITE_OK)
    {
        rsId = mDb.last_insert_rowid();
    }
    sqlite3_mutex_leave(mutex);

    if(ret == SQLITE_CONSTRAINT_UNIQUE)
    {
        //There is already an RS with no model set, use that instead
        query findEmpty(mDb, "SELECT id FROM rs_list WHERE model_id=0 OR model_id IS NULL LIMIT 1");
        if(findEmpty.step() == SQLITE_ROW)
        {
            rsId = findEmpty.getRows().get<db_id>(0);
        }
    }
    else if(ret != SQLITE_OK)
    {
        if(errOut)
            *errOut = mDb.error_msg();
        qDebug() << "RollingstockSQLModel Error adding:" << ret << mDb.error_msg() << mDb.error_code() << mDb.extended_error_code();
    }

    //Clear filters
    m_modelFilter.clear();
    m_modelFilter.squeeze();
    m_numberFilter.clear();
    m_numberFilter.squeeze();
    m_ownerFilter.clear();
    m_ownerFilter.squeeze();
    emit filterChanged();

    refreshData(); //Recalc row count
    switchToPage(0); //Reset to first page and so it is shown as first row

    if(outRow)
        *outRow = rsId ? 0 : -1; //Empty model is always the first

    return rsId;
}

bool RollingstockSQLModel::removeAllRS()
{
    command cmd(mDb, "DELETE FROM rs_list");
    int ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        qWarning() << "Removing ALL RS:" << ret << mDb.extended_error_code() << "Err:" << mDb.error_msg();
        return false;
    }

    refreshData(); //Recalc row count
    return true;
}

qint64 RollingstockSQLModel::recalcTotalItemCount()
{
    query q(mDb);
    buildQuery(q, 0, 0, false);

    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void RollingstockSQLModel::buildQuery(sqlite3pp::query &q, int sortCol, int offset, bool fullData)
{
    QByteArray sql;
    if(fullData)
    {
        sql = "SELECT rs_list.id,rs_list.number,rs_list.model_id,rs_list.owner_id,"
              "rs_models.name,rs_models.suffix,rs_models.type,rs_owners.name"
              " FROM rs_list";
    }
    else
    {
        sql = "SELECT COUNT(1) FROM rs_list";
    }

    //If counting but filtering by model name (not null) we need to JOIN rs_models
    bool modelIsNull = m_modelFilter.startsWith(nullFilterStr, Qt::CaseInsensitive);
    if(fullData || (!modelIsNull && !m_modelFilter.isEmpty()))
        sql += " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id";

    //If counting but filtering by owner name (not null) we need to JOIN rs_owners
    bool ownerIsNull = m_ownerFilter.startsWith(nullFilterStr, Qt::CaseInsensitive);
    if(fullData || (!ownerIsNull && !m_ownerFilter.isEmpty()))
        sql += " LEFT JOIN rs_owners ON rs_owners.id=rs_list.owner_id";

    bool whereClauseAdded = false;

    if(!m_modelFilter.isEmpty())
    {
        sql.append(" WHERE ");

        if(modelIsNull)
        {
            sql.append("rs_list.model_id IS NULL");
        }
        else
        {
            sql.append("rs_models.name LIKE ?3");
        }

        whereClauseAdded = true;
    }

    if(!m_numberFilter.isEmpty())
    {
        if(whereClauseAdded)
            sql.append(" AND ");
        else
            sql.append(" WHERE ");

        sql.append("rs_list.number LIKE ?4");

        whereClauseAdded = true;
    }

    if(!m_ownerFilter.isEmpty())
    {
        if(whereClauseAdded)
            sql.append(" AND ");
        else
            sql.append(" WHERE ");

        if(ownerIsNull)
        {
            sql.append("rs_list.owner_id IS NULL");
        }
        else
        {
            sql.append("rs_owners.name LIKE ?5");
        }
    }

    if(fullData)
    {
        //Apply sorting
        const char *sortColExpr = nullptr;
        switch (sortCol)
        {
        case Model:
        {
            sortColExpr = "rs_models.name,rs_list.number"; //Order by 2 columns, no where clause
            break;
        }
        case Owner:
        {
            sortColExpr = "rs_owners.name,rs_models.name,rs_list.number"; //Order by 3 columns, no where clause
            break;
        }
        case TypeCol:
        {
            sortColExpr = "rs_models.type,rs_models.name,rs_list.number"; //Order by 3 columns, no where clause
            break;
        }
        }

        sql += " ORDER BY ";
        sql += sortColExpr;

        sql += " LIMIT ?1";
        if(offset)
            sql += " OFFSET ?2";
    }

    q.prepare(sql);

    if(fullData)
    {
        //Apply offset and batch size
        q.bind(1, BatchSize);
        if(offset)
            q.bind(2, offset);
    }

    //Apply filters
    QByteArray modelFilter;
    if(!m_modelFilter.isEmpty() && !modelIsNull)
    {
        modelFilter.reserve(m_modelFilter.size() + 2);
        modelFilter.append('%');
        modelFilter.append(m_modelFilter.toUtf8());
        modelFilter.append('%');

        sqlite3_bind_text(q.stmt(), 3, modelFilter, modelFilter.size(), SQLITE_STATIC);
    }

    QByteArray numberFilter;
    if(!m_numberFilter.isEmpty())
    {
        numberFilter.reserve(m_numberFilter.size() + 2);
        numberFilter.append('%');
        numberFilter.append(m_numberFilter.toUtf8());
        numberFilter.append('%');
        numberFilter.replace('-', nullptr); //Remove dashes

        sqlite3_bind_text(q.stmt(), 4, numberFilter, numberFilter.size(), SQLITE_STATIC);
    }

    QByteArray ownerFilter;
    if(!m_ownerFilter.isEmpty() && !ownerIsNull)
    {
        ownerFilter.reserve(m_ownerFilter.size() + 2);
        ownerFilter.append('%');
        ownerFilter.append(m_ownerFilter.toUtf8());
        ownerFilter.append('%');

        sqlite3_bind_text(q.stmt(), 5, ownerFilter, ownerFilter.size(), SQLITE_STATIC);
    }
}

void RollingstockSQLModel::internalFetch(int first, int sortCol, int /*valRow*/, const QVariant& /*val*/)
{
    query q(mDb);

    int offset = first + curPage * ItemsPerPage;

    qDebug() << "Fetching:" << first << "Offset:" << offset;

    buildQuery(q, sortCol, offset, true);

    QVector<RSItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    int i = 0;
    for(; it != end; ++it)
    {
        auto r = *it;
        RSItem &item = vec[i];
        item.rsId = r.get<db_id>(0);
        item.number = r.get<int>(1);
        item.modelId = r.get<db_id>(2);
        item.ownerId = r.get<db_id>(3);
        item.modelName = r.get<QString>(4);
        item.modelSuffix = r.get<QString>(5);
        item.type = RsType(r.get<int>(6));
        item.ownerName = r.get<QString>(7);

        i += 1;
    }

    if(i < BatchSize)
        vec.remove(i, BatchSize - i);

    postResult(vec, first);
}

bool RollingstockSQLModel::setModel(RSItem &item, db_id modelId, const QString &name)
{
    if(item.modelId == modelId)
        return false;

    //FIXME: should be already handled by UNIQUE constraints
    //Check if there is already that combination of
    //Model.Number, if so set an higher number
    query q_hasModelNumCombination(mDb, "SELECT id, model_id, number FROM rs_list WHERE model_id=? AND number=?");
    q_hasModelNumCombination.bind(1, modelId);
    q_hasModelNumCombination.bind(2, item.number);
    int ret = q_hasModelNumCombination.step();
    q_hasModelNumCombination.reset();

    if(ret == SQLITE_ROW)
    {
        //There's already that Model.Number, change our number
        query q_getMaxNumberOfThatModel(mDb, "SELECT MAX(number) FROM rs_list WHERE model_id=?");
        q_getMaxNumberOfThatModel.bind(1, modelId);
        q_getMaxNumberOfThatModel.step();
        auto r = q_getMaxNumberOfThatModel.getRows();
        int number = r.get<int>(0) + 1; //Max + 1
        q_getMaxNumberOfThatModel.reset();

        //BIG TODO: numeri carri/carrozze hanno cifra di controllo, non posono essere aumentati di +1 a caso
        //ERRORMSG: ask user

        command q_setNumber(mDb, "UPDATE rs_list SET number=? WHERE id=?");
        q_setNumber.bind(1, number);
        q_setNumber.bind(2, item.rsId);
        ret = q_setNumber.execute();
        q_setNumber.reset();

        if(ret != SQLITE_OK)
            return false;

        item.number = number;
    }

    command q_setModel(mDb, "UPDATE rs_list SET model_id=? WHERE id=?");
    q_setModel.bind(1, modelId);
    q_setModel.bind(2, item.rsId);
    ret = q_setModel.execute();
    q_setModel.reset();

    if(ret != SQLITE_OK)
        return false;

    item.modelId = modelId;
    Q_UNUSED(name) //We clear the cache so the name will be re-queried

    //This row has now changed position so we need to invalidate cache
    //HACK: we emit dataChanged for this index (that doesn't exist anymore)
    //but the view will trigger fetching at same scroll position so it is enough
    cache.clear();
    cacheFirstRow = 0;

    emit Session->rollingStockModified(item.rsId);

    return true;
}

bool RollingstockSQLModel::setOwner(RSItem &item, db_id ownerId, const QString& name)
{
    if(item.ownerId == ownerId)
        return false;

    command q_setOwner(mDb, "UPDATE rs_list SET owner_id=? WHERE id=?");
    q_setOwner.bind(1, ownerId);
    q_setOwner.bind(2, item.rsId);
    int ret = q_setOwner.execute();
    q_setOwner.reset();

    if(ret != SQLITE_OK)
        return false;

    item.ownerId = ownerId;
    item.ownerName = name;

    emit Session->rollingStockModified(item.rsId);

    if(sortColumn == Owner)
    {
        //This row has now changed position so we need to invalidate cache
        //HACK: we emit dataChanged for this index (that doesn't exist anymore)
        //but the view will trigger fetching at same scroll position so it is enough
        cache.clear();
        cacheFirstRow = 0;
    }

    return true;
}

bool RollingstockSQLModel::setNumber(RSItem &item, int number)
{
    if(item.number == number)
        return false;

    //Check if there is already that combination of
    //Model.Number, if so don't set new number
    //ERRORMSG: show error to user (emit error(int code, QString msg))
    //TODO: use UNIQUE constraint???
    command q_hasModelNumCombination(mDb, "SELECT id,model_id,number FROM rs_list WHERE model_id=? AND number=?");
    q_hasModelNumCombination.bind(1, item.modelId);
    q_hasModelNumCombination.bind(2, number);
    int ret = q_hasModelNumCombination.step();
    q_hasModelNumCombination.reset();

    if(ret == SQLITE_ROW) //We already have one
        return false;

    command q_setNumber(mDb, "UPDATE rs_list SET number=? WHERE id=?");
    q_setNumber.bind(1, number);
    q_setNumber.bind(2, item.rsId);
    ret = q_setNumber.execute();
    q_setNumber.reset();

    if(ret != SQLITE_OK)
        return false;

    item.number = number;
    //This row has now changed position so we need to invalidate cache
    //HACK: we emit dataChanged for this index (that doesn't exist anymore)
    //but the view will trigger fetching at same scroll position so it is enough
    cache.clear();
    cacheFirstRow = 0;

    emit Session->rollingStockModified(item.rsId);

    return true;
}
