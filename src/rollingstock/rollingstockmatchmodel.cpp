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

#include "rollingstockmatchmodel.h"

#include "utils/rs_utils.h"

RollingstockMatchModel::RollingstockMatchModel(database &db, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    q_getMatches(mDb)
{
    regExp.setPattern("(?<model>[\\w\\s-]*)\\s*\\.?\\s*(?<num>\\d*)\\s*(:(?<owner>[\\w\\s-]+)?)?\\w*");
    regExp.optimize();
    q_getMatches.prepare("SELECT rs_list.id,rs_list.number,rs_models.name,rs_models.suffix,rs_models.type,rs_owners.name,"
                         "(CASE WHEN rs_list.number LIKE ?1 THEN 2 ELSE 0 END +"
                         " CASE WHEN rs_models.name LIKE ?2 THEN 1 ELSE 0 END +"
                         " CASE WHEN rs_models.suffix LIKE ?2 THEN 1 ELSE 0 END +"
                         " CASE WHEN rs_owners.name LIKE ?3 THEN 3 ELSE 0 END) AS s"
                         " FROM rs_list"
                         " JOIN rs_models ON rs_models.id=rs_list.model_id"
                         " JOIN rs_owners ON rs_owners.id=rs_list.owner_id"
                         " ORDER BY s DESC LIMIT " QT_STRINGIFY(MaxMatchItems + 1));
    //FIXME: non funziona bene, i risultati sembrano casuali
}

int RollingstockMatchModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant RollingstockMatchModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= size)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(isEmptyRow(idx.row()))
        {
            return idx.column() == NumberCol ? ISqlFKMatchModel::tr("Empty") : QVariant();
        }
        else if(isEllipsesRow(idx.row()))
        {
            return ellipsesString;
        }

        const RSItem& item = items[idx.row()];

        switch (idx.column())
        {
        case ModelCol:
            return item.modelName;
        case NumberCol:
            return rs_utils::formatNum(item.type, item.number);
        case SuffixCol:
            return item.modelSuffix;
        case OwnerCol:
            return item.ownerName;
        }
        break;
    }
    case Qt::FontRole:
    {
        if(isEmptyRow(idx.row()) || idx.column() == OwnerCol)
        {
            return boldFont();
        }
        break;
    }
    }

    return QVariant();
}

void RollingstockMatchModel::autoSuggest(const QString &text)
{
    QRegularExpressionMatch match = regExp.match(text);

    QString tmp = match.captured("model").toUtf8();
    model.clear();
    if(!tmp.isEmpty())
    {
        model.reserve(tmp.size() + 2);
        model.append('%');
        model.append(tmp.toUtf8());
        model.append('%');
    }

    tmp = match.captured("num").toUtf8();
    number.clear();
    if(!tmp.isEmpty())
    {
        number.reserve(tmp.size() + 2);
        number.append('%');
        number.append(tmp.toUtf8());
        number.append('%');
    }

    tmp = match.captured("owner").toUtf8();
    owner.clear();
    if(!tmp.isEmpty())
    {
        owner.reserve(tmp.size() + 2);
        owner.append('%');
        owner.append(tmp.toUtf8());
        owner.append('%');
    }

    refreshData();
}

void RollingstockMatchModel::refreshData()
{
    if(!mDb.db())
        return;

    beginResetModel();

    if(number.isEmpty())
        sqlite3_bind_null(q_getMatches.stmt(), 1);
    else
        sqlite3_bind_text(q_getMatches.stmt(), 1, number, number.size(), SQLITE_STATIC);

    if(model.isEmpty())
        sqlite3_bind_null(q_getMatches.stmt(), 2);
    else
        sqlite3_bind_text(q_getMatches.stmt(), 2, model, model.size(), SQLITE_STATIC);

    if(owner.isEmpty())
        sqlite3_bind_null(q_getMatches.stmt(), 3);
    else
        sqlite3_bind_text(q_getMatches.stmt(), 3, owner, owner.size(), SQLITE_STATIC);


    int i = 0;
    int ret = SQLITE_OK;
    while ((ret = q_getMatches.step() == SQLITE_ROW) && i < MaxMatchItems)
    {
        auto r = q_getMatches.getRows();
        RSItem &item = items[i];
        item.rsId = r.get<db_id>(0);
        item.number = r.get<int>(1);
        item.modelName = r.get<QString>(2);
        item.modelSuffix = r.get<QString>(3);
        item.type = RsType(r.get<int>(4));
        item.ownerName = r.get<QString>(5);
        i++;
    }

    size = i + 1; //Items + Empty

    if(ret == SQLITE_ROW)
    {
        //There would be still rows, show Ellipses
        size++; //Items + Empty + Ellispses
    }

    q_getMatches.reset();
    endResetModel();

    emit resultsReady(true);
}

QString RollingstockMatchModel::getName(db_id id) const
{
    if(!mDb.db())
        return QString();

    query q(mDb, "SELECT rs_list.number,rs_models.name,rs_models.suffix,rs_models.type"
                 " FROM rs_list JOIN rs_models ON rs_models.id=rs_list.model_id"
                 " WHERE rs_list.id=?");
    q.bind(1, id);
    if(q.step() != SQLITE_ROW)
        return QString();

    int num = sqlite3_column_int(q.stmt(), 0);
    int modelNameLen = sqlite3_column_bytes(q.stmt(), 1);
    const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(q.stmt(), 1));

    int modelSuffixLen = sqlite3_column_bytes(q.stmt(), 2);
    const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(q.stmt(), 2));
    RsType type = RsType(sqlite3_column_int(q.stmt(), 3));

    return rs_utils::formatNameRef(modelName, modelNameLen, num, modelSuffix, modelSuffixLen, type);
}

db_id RollingstockMatchModel::getIdAtRow(int row) const
{
    return items[row].rsId;
}

QString RollingstockMatchModel::getNameAtRow(int row) const
{
    return rs_utils::formatName(items[row].modelName, items[row].number, items[row].modelSuffix, items[row].type);
}
