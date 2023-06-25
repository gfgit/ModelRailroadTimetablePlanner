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

#include "linesmatchmodel.h"

#include <QTimerEvent>

LinesMatchModel::LinesMatchModel(database &db, bool useTimer, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    q_getMatches(mDb)
{
    timerId = useTimer ? 0 : -1;
}

LinesMatchModel::~LinesMatchModel()
{
    if (timerId > 0)
    {
        killTimer(timerId);
        timerId = 0;
    }
}

QVariant LinesMatchModel::data(const QModelIndex &idx, int role) const
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

        return items[idx.row()].name;
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

void LinesMatchModel::autoSuggest(const QString &text)
{
    mQuery.clear();
    if (!text.isEmpty())
    {
        mQuery.reserve(text.size() + 2);
        mQuery.append('%');
        mQuery.append(text.toUtf8());
        mQuery.append('%');
    }

    refreshData();
}

void LinesMatchModel::refreshData()
{
    if (!mDb.db())
        return;

    if (!q_getMatches.stmt())
        q_getMatches.prepare(
          "SELECT id,name FROM lines WHERE name LIKE ?1 LIMIT " QT_STRINGIFY(MaxMatchItems + 1));

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
        items[i].lineId = (*it).get<db_id>(0);
        items[i].name   = (*it).get<QString>(1);
        ++it;
    }

    size = i;

    if (hasEmptyRow)
        size++; // Items + Empty, add 1 row

    if (it != end)
    {
        // There would be still rows, show Ellipses
        size++; // Items + Empty + Ellispses
    }

    q_getMatches.reset();
    endResetModel();

    emit resultsReady(false);

    if (timerId < 0)
        return; // Do not use timer
    if (timerId == 0)
        timerId = startTimer(3000);
    timer.start();
}

void LinesMatchModel::clearCache()
{
    beginResetModel();
    size = 0;
    endResetModel();
}

QString LinesMatchModel::getName(db_id id) const
{
    if (!mDb.db())
        return QString();

    query q(mDb, "SELECT name FROM lines WHERE id=?");
    q.bind(1, id);
    if (q.step() == SQLITE_ROW)
        return q.getRows().get<QString>(0);
    return QString();
}

db_id LinesMatchModel::getIdAtRow(int row) const
{
    return items[row].lineId;
}

QString LinesMatchModel::getNameAtRow(int row) const
{
    return items[row].name;
}

void LinesMatchModel::timerEvent(QTimerEvent *e)
{
    if (timerId > 0 && e->timerId() == timerId)
    {
        if (timer.isValid() && timer.elapsed() < 3000)
            return; // Do another round

        killTimer(timerId);
        timerId = 0;
        q_getMatches.finish();
        return;
    }

    ISqlFKMatchModel::timerEvent(e);
}
