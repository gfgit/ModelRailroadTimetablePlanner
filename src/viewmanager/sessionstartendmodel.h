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

#ifndef SESSIONSTARTENDMODEL_H
#define SESSIONSTARTENDMODEL_H

#include <QAbstractItemModel>

#include <QList>
#include <QTime>

#include "utils/types.h"
#include "utils/session_rs_modes.h"

namespace sqlite3pp {
class database;
}

// FIXME: make incremental loading, cached like other on-demand models
class SessionStartEndModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    // Station or RS Owner
    struct ParentItem
    {
        db_id id;
        QString name;
        int firstIdx; // First RS index
    };

    struct RSItem
    {
        db_id rsId;
        db_id jobId;
        db_id stationOrOwnerId;
        QString platform;
        QString rsName;
        QString stationOrOwnerName;
        int parentIdx;
        QTime time;
        JobCategory jobCategory;
    };

    enum Columns
    {
        RsNameCol = 0,
        JobCol,
        PlatfCol,
        DepartureCol,
        StationOrOwnerCol,
        NCols
    };

    SessionStartEndModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &idx) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;

    Qt::ItemFlags flags(const QModelIndex &idx) const override;

    void setMode(SessionRSMode m, SessionRSOrder o, bool forceReload = false);

    inline SessionRSMode mode() const
    {
        return m_mode;
    }
    inline SessionRSOrder order() const
    {
        return m_order;
    }

private:
    sqlite3pp::database &mDb;

    QList<ParentItem> parents;
    QList<RSItem> rsData;

    SessionRSMode m_mode;
    SessionRSOrder m_order;
};

#endif // SESSIONSTARTENDMODEL_H
