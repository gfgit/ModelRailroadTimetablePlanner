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

#include "rsmodelsmatchmodel.h"

RSModelsMatchModel::RSModelsMatchModel(database &db, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    q_getMatches(mDb)
{
    q_getMatches.prepare("SELECT id,name,suffix FROM rs_models WHERE name LIKE ?1 OR suffix LIKE "
                         "?1 LIMIT " QT_STRINGIFY(MaxMatchItems + 1));
}

QVariant RSModelsMatchModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= size)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
    {
        if (isEmptyRow(idx.row()))
        {
            return ISqlFKMatchModel::tr("Empty");
        }
        else if (isEllipsesRow(idx.row()))
        {
            return ellipsesString;
        }

        return items[idx.row()].nameWithSuffix;
    }
    case Qt::FontRole:
    {
        if (isEmptyRow(idx.row()))
        {
            return boldFont();
        }
        break;
    }
    }

    return QVariant();
}

void RSModelsMatchModel::autoSuggest(const QString &text)
{
    mQuery.clear();
    if (!text.isEmpty())
    {
        mQuery.clear();
        mQuery.reserve(text.size() + 2);
        mQuery.append('%');
        mQuery.append(text.toUtf8());
        mQuery.append('%');
    }

    refreshData();
}

void RSModelsMatchModel::refreshData()
{
    if (!mDb.db())
        return;

    beginResetModel();

    char emptyQuery = '%';

    if (mQuery.isEmpty())
        sqlite3_bind_text(q_getMatches.stmt(), 1, &emptyQuery, 1, SQLITE_STATIC);
    else
        sqlite3_bind_text(q_getMatches.stmt(), 1, mQuery, mQuery.size(), SQLITE_STATIC);

    auto end = q_getMatches.end();
    auto it  = q_getMatches.begin();
    int i    = 0;
    for (; i < MaxMatchItems && it != end; i++)
    {
        items[i].modelId    = (*it).get<db_id>(0);

        items[i].nameLength = sqlite3_column_bytes(q_getMatches.stmt(), 1);
        const char *name =
          reinterpret_cast<const char *>(sqlite3_column_text(q_getMatches.stmt(), 1));

        int suffixLength = sqlite3_column_bytes(q_getMatches.stmt(), 2);
        const char *suffix =
          reinterpret_cast<const char *>(sqlite3_column_text(q_getMatches.stmt(), 2));

        if (suffix && suffixLength)
        {
            QByteArray buf;
            buf.reserve(items[i].nameLength + suffixLength + 3);
            buf.append(name, items[i].nameLength);
            buf.append(" (", 2);
            buf.append(suffix, suffixLength);
            buf.append(')');
            items[i].nameWithSuffix = QString::fromUtf8(buf);
        }
        else
        {
            items[i].nameWithSuffix = QString::fromUtf8(name, items[i].nameLength);
        }
        ++it;
    }

    size = i + 1; // Items + Empty

    if (it != end)
    {
        // There would be still rows, show Ellipses
        size++; // Items + Empty + Ellispses
    }

    q_getMatches.reset();
    endResetModel();

    emit resultsReady(false);
}

QString RSModelsMatchModel::getName(db_id id) const
{
    if (!mDb.db())
        return QString();

    query q(mDb, "SELECT name FROM rs_models WHERE id=?");
    q.bind(1, id);
    if (q.step() == SQLITE_ROW)
        return q.getRows().get<QString>(0);
    return QString();
}

db_id RSModelsMatchModel::getIdAtRow(int row) const
{
    return items[row].modelId;
}

QString RSModelsMatchModel::getNameAtRow(int row) const
{
    return items[row].nameWithSuffix.left(
      items[row].nameLength); // Remove the suffix, keep only the name
}
