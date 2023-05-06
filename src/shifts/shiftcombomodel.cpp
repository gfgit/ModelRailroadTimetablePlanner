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

#include "shiftcombomodel.h"

#include "app/session.h"

#include <QDebug>

#include <QCoreApplication>
#include "shifts/shiftresultevent.h"

ShiftComboModel::ShiftComboModel(database &db, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    totalCount(0),
    isFetching(false)
{
}

bool ShiftComboModel::event(QEvent *e)
{
    if(e->type() == ShiftResultEvent::_Type)
    {
        ShiftResultEvent *ev = static_cast<ShiftResultEvent *>(e);
        ev->setAccepted(true);

        isFetching = false;
        if(totalCount < 0)
            totalCount = -totalCount; //Now is positive

        QModelIndex firstIdx = index(0, 0);
        QModelIndex lastIdx = index(size - 1, NCols - 1);
        emit dataChanged(firstIdx, lastIdx);

        emit resultsReady(false);

        return true;
    }

    return ISqlFKMatchModel::event(e);
}

void ShiftComboModel::refreshData()
{
    int count = recalcRowCount();

    if(count != totalCount) //Invalidate cache and reset model
    {
        beginResetModel();
    }

    if(count > MaxMatchItems)
    {
        size = MaxMatchItems + 2; //MaxItems + Empty + '...'
    }else{
        size = count + 1; //Items + Empty
    }

    if(count != totalCount)
    {
        endResetModel();
    }

    totalCount = -count; //Negative, clears cache
}

void ShiftComboModel::clearCache()
{
    if(totalCount > 0)
        totalCount = -totalCount;
}

int ShiftComboModel::recalcRowCount()
{
    if(!mDb.db())
        return 0;
    query q(mDb);
    if(mQuery.isEmpty())
    {
        q.prepare("SELECT COUNT(1) FROM jobshifts");
    }else{
        q.prepare("SELECT COUNT(1) FROM jobshifts WHERE name LIKE ?");
        q.bind(1, mQuery);
    }
    q.step();
    const int count = q.getRows().get<int>(0);
    return count;
}

void ShiftComboModel::fetchRow()
{
    if(!mDb.db() || isFetching)
        return; //Already fetching

    //TODO: use a custom QRunnable
    /*
    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection);
                              */
    internalFetch();
}

void ShiftComboModel::internalFetch()
{
    if(!mDb.db())
    {
        //Error: database not open, empty result
        ShiftResultEvent *ev = new ShiftResultEvent;
        qApp->postEvent(this, ev);
        return;
    }

    query q(mDb);

    QByteArray sql = "SELECT id,name FROM jobshifts ";

    if(!mQuery.isEmpty())
        sql += "WHERE name LIKE ? ";

    sql += "ORDER BY name LIMIT " QT_STRINGIFY(MaxMatchItems);

    q.prepare(sql);

    if(!mQuery.isEmpty())
        q.bind(1, mQuery);

    auto it = q.begin();
    const auto end = q.end();

    int i = 0;
    for(; it != end && i < MaxMatchItems; ++it)
    {
        auto r = *it;
        items[i].shiftId = r.get<db_id>(0);
        items[i].name = r.get<QString>(1);
        i++;
    }

    ShiftResultEvent *ev = new ShiftResultEvent;
    qApp->postEvent(this, ev);
}

QVariant ShiftComboModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= size)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(totalCount < 0)
        {
            //Fetch above or below current cach
            const_cast<ShiftComboModel *>(this)->fetchRow();

            //Temporarily return null
            return QVariant();
        }

        if(isEmptyRow(idx.row()))
        {
            return ISqlFKMatchModel::tr("Empty");
        }
        else if(isEllipsesRow(idx.row()))
        {
            return ellipsesString;
        }

        if(isFetching)
            break; //Don't acces data while fetching

        return items[idx.row()].name;
    }
    case Qt::FontRole:
    {
        if(isEmptyRow(idx.row()))
        {
            return boldFont();
        }
        break;
    }
    }

    return QVariant();
}

void ShiftComboModel::autoSuggest(const QString &text)
{
    mQuery.clear();
    if(!text.isEmpty())
    {
        mQuery.clear();
        mQuery.reserve(text.size() + 2);
        mQuery.append('%');
        mQuery.append(text.toUtf8());
        mQuery.append('%');
    }

    refreshData();
}

QString ShiftComboModel::getName(db_id shiftId) const
{
    if(!mDb.db())
        return QString(); //Error: database not open

    QString shiftName;
    query q_getShift(mDb, "SELECT name FROM jobshifts WHERE id=?");
    q_getShift.bind(1, shiftId);
    if(q_getShift.step() == SQLITE_ROW)
    {
        auto r = q_getShift.getRows();
        shiftName = r.get<QString>(0);
    }
    return shiftName;
}

db_id ShiftComboModel::getIdAtRow(int row) const
{
    return items[row].shiftId;
}

QString ShiftComboModel::getNameAtRow(int row) const
{
    return items[row].name;
}
