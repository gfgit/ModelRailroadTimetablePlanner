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

#include "linesmodel.h"

#include "utils/delegates/sql/pageditemmodelhelper_impl.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/delegates/kmspinbox/kmutils.h"

#include <QDebug>

// Error messages
static constexpr char errorNameAlreadyUsedText[] =
  QT_TRANSLATE_NOOP("LinesModel", "The name <b>%1</b> is already used by another line.<br>"
                                  "Please choose a different name for each railway line.");

static constexpr char errorLineInUseText[] =
  QT_TRANSLATE_NOOP("LinesModel", "Cannot delete <b>%1</b> line because it is stille referenced.");

LinesModel::LinesModel(sqlite3pp::database &db, QObject *parent) :
    BaseClass(500, db, parent)
{
    sortColumn = NameCol;
}

QVariant LinesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (section)
            {
            case NameCol:
                return tr("Name");
            case StartKm:
                return tr("Sart At Km");
            }
            break;
        }
        }
    }
    else if (role == Qt::DisplayRole)
    {
        return section + curPage * ItemsPerPage + 1;
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant LinesModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        // Fetch above or below current cache
        const_cast<LinesModel *>(this)->fetchRow(row);

        // Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const LineItem &item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case NameCol:
            return item.name;
        case StartKm:
            return utils::kmNumToText(item.startMeters);
        }
        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case NameCol:
            return item.name;
        case StartKm:
            return item.startMeters;
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        switch (idx.column())
        {
        case StartKm:
            return QVariant::fromValue(Qt::AlignRight | Qt::AlignVCenter);
        }
        break;
    }
    }

    return QVariant();
}

bool LinesModel::addLine(const QString &name, db_id *outLineId)
{
    if (name.isEmpty())
        return false;

    command q_newStation(mDb, "INSERT INTO lines(id,name,start_meters)"
                              " VALUES (NULL, ?, 0)");
    q_newStation.bind(1, name);

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret      = q_newStation.execute();
    db_id lineId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_newStation.reset();

    if ((ret != SQLITE_OK && ret != SQLITE_DONE) || lineId == 0)
    {
        // Error
        if (outLineId)
            *outLineId = 0;

        if (ret == SQLITE_CONSTRAINT_UNIQUE)
        {
            emit modelError(tr(errorNameAlreadyUsedText).arg(name));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }
        return false;
    }

    if (outLineId)
        *outLineId = lineId;

    refreshData(); // Recalc row count
    setSortingColumn(NameCol);
    switchToPage(0); // Reset to first page and so it is shown as first row

    emit Session->lineAdded(lineId);

    return true;
}

bool LinesModel::removeLine(db_id lineId)
{
    command q_removeStation(mDb, "DELETE FROM lines WHERE id=?");

    q_removeStation.bind(1, lineId);
    int ret = q_removeStation.execute();
    q_removeStation.reset();

    if (ret != SQLITE_OK)
    {
        if (ret == SQLITE_CONSTRAINT_FOREIGNKEY || ret == SQLITE_CONSTRAINT_TRIGGER)
        {
            // TODO: show more information to the user, like where it's still referenced
            query q(mDb, "SELECT name FROM lines WHERE id=?");
            q.bind(1, lineId);
            if (q.step() == SQLITE_ROW)
            {
                const QString name = q.getRows().get<QString>(0);
                emit modelError(tr(errorLineInUseText).arg(name));
            }
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }

        return false;
    }

    emit Session->lineRemoved(lineId);

    refreshData(); // Recalc row count

    return true;
}

qint64 LinesModel::recalcTotalItemCount()
{
    // TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM lines");
    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void LinesModel::internalFetch(int first, int sortCol, int valRow, const QVariant &val)
{
    query q(mDb);

    int offset   = first - valRow + curPage * ItemsPerPage;
    bool reverse = false;

    if (valRow > first)
    {
        offset  = 0;
        reverse = true;
    }

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset
             << "Reverse:" << reverse;

    const char *whereCol = nullptr;

    QByteArray sql       = "SELECT id,name,start_meters FROM lines";
    switch (sortCol)
    {
    case NameCol:
    {
        whereCol = "name"; // Order by 1 column, no where clause
        break;
    }
    }

    if (val.isValid())
    {
        sql += " WHERE ";
        sql += whereCol;
        if (reverse)
            sql += "<?3";
        else
            sql += ">?3";
    }

    sql += " ORDER BY ";
    sql += whereCol;

    if (reverse)
        sql += " DESC";

    sql += " LIMIT ?1";
    if (offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    if (offset)
        q.bind(2, offset);

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

    QList<LineItem> vec(BatchSize);

    auto it             = q.begin();
    const auto end      = q.end();

    int i               = reverse ? BatchSize - 1 : 0;
    const int increment = reverse ? -1 : 1;

    for (; it != end; ++it)
    {
        auto r           = *it;
        LineItem &item   = vec[i];
        item.lineId      = r.get<db_id>(0);
        item.name        = r.get<QString>(1);
        item.startMeters = r.get<int>(2);

        i += increment;
    }

    if (reverse && i > -1)
        vec.remove(0, i + 1);
    else if (i < BatchSize)
        vec.remove(i, BatchSize - i);

    postResult(vec, first);
}
