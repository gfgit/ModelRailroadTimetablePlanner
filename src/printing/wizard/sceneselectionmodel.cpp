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

#include "sceneselectionmodel.h"

#include "graph/model/linegraphscene.h"

SceneSelectionModel::SceneSelectionModel(sqlite3pp::database &db, QObject *parent) :
    QAbstractTableModel(parent),
    mDb(db),
    mQuery(mDb),
    selectedType(LineGraphType::NoGraph),
    selectionMode(UseSelectedEntries),
    cachedCount(-1),
    iterationIdx(-1)
{
}

QVariant SceneSelectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (section)
            {
            case TypeCol:
                return tr("Type");
            case NameCol:
                return tr("Name");
            }
            break;
        }
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

int SceneSelectionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : entries.count();
}

int SceneSelectionModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant SceneSelectionModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= entries.size() || role != Qt::DisplayRole)
        return QVariant();

    const Entry &entry = entries.at(idx.row());

    switch (idx.column())
    {
    case TypeCol:
    {
        return utils::getLineGraphTypeName(entry.type);
    }
    case NameCol:
    {
        return entry.name;
    }
    }

    return QVariant();
}

bool SceneSelectionModel::addEntry(const Entry &entry)
{
    if (selectionMode == AllOfTypeExceptSelected && entry.type != selectedType)
        return false; // Not the right type

    for (const Entry &e : qAsConst(entries))
    {
        if (e.objectId == entry.objectId && e.type == entry.type)
            return false; // Already added
    }

    const int row = entries.size();
    beginInsertRows(QModelIndex(), row, row);
    entries.append(entry);
    endInsertRows();

    cachedCount = -1;
    emit selectionCountChanged();

    return true;
}

void SceneSelectionModel::removeAt(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    entries.removeAt(row);
    endRemoveRows();

    cachedCount = -1;
    emit selectionCountChanged();
}

void SceneSelectionModel::moveRow(int row, bool up)
{
    if (row == 0 && !up)
        return;
    if (row == entries.size() - 1 && up)
        return;

    const int dest = up ? row + 2 : row - 1;

    beginMoveRows(QModelIndex(), row, row, QModelIndex(), dest);
    entries.move(row, dest);
    endMoveRows();
}

void SceneSelectionModel::setMode(SelectionMode mode, LineGraphType type)
{
    if (mode == UseSelectedEntries)
        type = LineGraphType::NoGraph;
    else if (type == LineGraphType::NoGraph)
        return; // Must set a valid type

    if (selectionMode == mode && selectedType == type)
        return;

    selectionMode = mode;
    selectedType  = type;

    if (selectionMode == AllOfTypeExceptSelected)
        keepOnlyType(selectedType);

    emit selectionModeChanged(int(mode), int(type));

    cachedCount = -1;
    emit selectionCountChanged();
}

qint64 SceneSelectionModel::getItemCount()
{
    if (cachedCount >= 0)
        return cachedCount;

    if (selectionMode == UseSelectedEntries)
    {
        cachedCount = entries.size();
    }
    else
    {
        QByteArray sql = "SELECT COUNT(id) FROM ";
        switch (selectedType)
        {
        case LineGraphType::SingleStation:
            sql.append("stations");
            break;
        case LineGraphType::RailwaySegment:
            sql.append("railway_segments");
            break;
        case LineGraphType::RailwayLine:
            sql.append("lines");
            break;
        default:
            return -1; // Error
        }

        sqlite3pp::query q(mDb);
        if (q.prepare(sql) != SQLITE_OK || q.step() != SQLITE_ROW)
            return -1;

        qint64 totalCount = q.getRows().get<qint64>(0);
        totalCount -= entries.size(); // "Except" selected
        if (totalCount < 0)
            totalCount = 0;

        cachedCount = totalCount;
    }

    return cachedCount;
}

bool SceneSelectionModel::startIteration()
{
    if (selectionMode == UseSelectedEntries)
    {
        iterationIdx = 0;
        return true;
    }

    QByteArray sql = "SELECT id FROM ";
    switch (selectedType)
    {
    case LineGraphType::SingleStation:
        sql.append("stations");
        break;
    case LineGraphType::RailwaySegment:
        sql.append("railway_segments");
        break;
    case LineGraphType::RailwayLine:
        sql.append("lines");
        break;
    default:
        return false; // Error
    }

    int ret = mQuery.prepare(sql);
    return ret == SQLITE_OK;
}

IGraphSceneCollection::SceneItem SceneSelectionModel::getNextItem()
{
    SceneItem item;

    Entry entry = getNextEntry();
    if (!entry.objectId)
        return item;

    // Create new scene without parent so ownership is passed to caller
    LineGraphScene *lineScene = new LineGraphScene(mDb);
    lineScene->loadGraph(entry.objectId, entry.type);

    item.scene = lineScene;
    item.name  = lineScene->getGraphObjectName();
    item.type  = utils::getLineGraphTypeName(entry.type);
    return item;
}

QString SceneSelectionModel::getModeName(SelectionMode mode)
{
    switch (mode)
    {
    case UseSelectedEntries:
        return tr("Select items");
    case AllOfTypeExceptSelected:
        return tr("All except selected items");
    default:
        break;
    }
    return QString();
}

void SceneSelectionModel::removeAll()
{
    beginResetModel();
    entries.clear();
    entries.squeeze();
    endResetModel();

    cachedCount = -1;
    emit selectionCountChanged();
}

SceneSelectionModel::Entry SceneSelectionModel::getNextEntry()
{
    Entry entry{0, QString(), selectedType};

    if (selectionMode == UseSelectedEntries)
    {
        if (iterationIdx < 0 || iterationIdx >= entries.size())
            return entry;

        entry = entries.at(iterationIdx);
        iterationIdx++;

        if (iterationIdx == entries.size())
            iterationIdx = -1; // End iteration
    }
    else
    {
        if (!mQuery.stmt())
            return entry; // Error: not iteration not started

        while (true)
        {
            int ret = mQuery.step();
            if (ret != SQLITE_ROW)
            {
                if (ret == SQLITE_OK || ret == SQLITE_DONE)
                    mQuery.finish(); // End iteration
                return entry;
            }

            entry.objectId = mQuery.getRows().get<db_id>(0);

            bool exclude   = false;
            for (const Entry &e : qAsConst(entries))
            {
                if (e.objectId == entry.objectId)
                {
                    exclude = true;
                    break;
                }
            }

            if (!exclude)
                break;
        }
    }

    return entry;
}

void SceneSelectionModel::keepOnlyType(LineGraphType type)
{
    beginResetModel();

    auto it = entries.begin();
    while (it != entries.end())
    {
        if (it->type != type)
            it = entries.erase(it);
        else
            it++;
    }

    endResetModel();
}
