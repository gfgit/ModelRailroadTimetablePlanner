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

#include "stationtracksmatchmodel.h"

#include <QBrush>

using namespace sqlite3pp;

StationTracksMatchModel::StationTracksMatchModel(sqlite3pp::database &db, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    q_getMatches(mDb, "SELECT id,type,platf_length_cm,freight_length_cm,name"
                      " FROM station_tracks WHERE station_id=?2 AND name LIKE ?1"
                      " ORDER BY pos"),
    m_stationId(0)
{
}

QVariant StationTracksMatchModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= size)
        return QVariant();

    const bool emptyRow =
      hasEmptyRow && (idx.row() == ItemCount || (size < ItemCount + 2 && idx.row() == size - 1));
    const bool ellipsesRow = idx.row() == (ItemCount - 1 - (hasEmptyRow ? 0 : 1));

    switch (role)
    {
    case Qt::DisplayRole:
    {
        if (emptyRow)
        {
            return ISqlFKMatchModel::tr("Empty");
        }
        else if (ellipsesRow)
        {
            return ellipsesString;
        }

        return items[idx.row()].name;
    }
    case Qt::ToolTipRole:
    {
        if (!emptyRow && !ellipsesRow)
        {
            QString typeStr;
            if (items[idx.row()].passenger && items[idx.row()].freight)
                typeStr = tr("All");
            else if (items[idx.row()].passenger)
                typeStr = tr("Passenger");
            else if (items[idx.row()].freight)
                typeStr = tr("Freight");
            return tr("Track <b>%1</b> for %2 traffic.").arg(items[idx.row()].name, typeStr);
        }
        break;
    }
    case Qt::FontRole:
    {
        if (emptyRow)
        {
            return boldFont();
        }
        if (!ellipsesRow && items[idx.row()].type.testFlag(utils::StationTrackType::Through))
        {
            return boldFont();
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        if (!emptyRow && !ellipsesRow)
            return QVariant::fromValue(Qt::AlignRight | Qt::AlignVCenter);;
        break;
    }
    case Qt::BackgroundRole:
    {
        if (!emptyRow && !ellipsesRow)
        {
            QColor color;
            if (items[idx.row()].passenger && items[idx.row()].freight)
                color = Qt::red;
            else if (items[idx.row()].passenger)
                color = Qt::yellow;
            else if (items[idx.row()].freight)
                color = Qt::green;
            color.setAlpha(70);
            return QBrush(color);
        }
        break;
    }
    case Qt::DecorationRole:
    {
        if (!emptyRow && !ellipsesRow)
        {
            if (items[idx.row()].type.testFlag(utils::StationTrackType::Electrified))
                return QColor(Qt::blue);
        }
        break;
    }
    }

    return QVariant();
}

void StationTracksMatchModel::autoSuggest(const QString &text)
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

void StationTracksMatchModel::refreshData()
{
    if (!mDb.db())
        return;

    beginResetModel();

    char emptyQuery = '%';

    if (mQuery.isEmpty())
        sqlite3_bind_text(q_getMatches.stmt(), 1, &emptyQuery, 1, SQLITE_STATIC);
    else
        sqlite3_bind_text(q_getMatches.stmt(), 1, mQuery, mQuery.size(), SQLITE_STATIC);

    q_getMatches.bind(2, m_stationId);

    auto end = q_getMatches.end();
    auto it  = q_getMatches.begin();
    int i    = 0;
    for (; i < ItemCount && it != end; i++)
    {
        auto track         = *it;
        items[i].trackId   = track.get<db_id>(0);
        items[i].type      = utils::StationTrackType(track.get<int>(1));
        items[i].passenger = track.get<int>(2) != 0;
        items[i].freight   = track.get<int>(3) != 0;
        items[i].name      = track.get<QString>(4);
        ++it;
    }

    size = i;
    if (hasEmptyRow)
        size++; // Items + Empty

    if (it != end)
    {
        // There would be still rows, show Ellipses
        size++; // Items + Empty + Ellispses
    }

    q_getMatches.reset();
    endResetModel();

    emit resultsReady(false);
}

QString StationTracksMatchModel::getName(db_id id) const
{
    if (!mDb.db())
        return QString();

    query q(mDb, "SELECT name FROM station_tracks WHERE id=?");
    q.bind(1, id);
    if (q.step() == SQLITE_ROW)
        return q.getRows().get<QString>(0);
    return QString();
}

db_id StationTracksMatchModel::getIdAtRow(int row) const
{
    return items[row].trackId;
}

QString StationTracksMatchModel::getNameAtRow(int row) const
{
    return items[row].name;
}

void StationTracksMatchModel::setFilter(db_id stationId)
{
    m_stationId = stationId;
    refreshData();
}

StationTracksMatchFactory::StationTracksMatchFactory(database &db, QObject *parent) :
    IMatchModelFactory(parent),
    m_stationId(0),
    mDb(db)
{
}

ISqlFKMatchModel *StationTracksMatchFactory::createModel()
{
    StationTracksMatchModel *m = new StationTracksMatchModel(mDb);
    m->setFilter(m_stationId);
    return m;
}
