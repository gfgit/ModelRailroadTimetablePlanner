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

#ifndef RAILWAYSEGMENTCONNECTIONSMODEL_H
#define RAILWAYSEGMENTCONNECTIONSMODEL_H

#include <QAbstractTableModel>

#include <QVector>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class RailwaySegmentConnectionsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns {
        FromGateTrackCol = 0,
        ToGateTrackCol,
        NCols
    };

    enum ItemState {
        NoChange = 0,
        AddedButNotComplete,
        ToAdd,
        ToRemove,
        Edited
    };

    struct RailwayTrack
    {
        db_id connId;
        int fromTrack;
        int toTrack;
        ItemState state;
    };

    static constexpr const int InvalidTrack = -1;
    static constexpr const int NewTrackAdded = -1;
    static constexpr const int TrackAlreadyConnected = -2;

    RailwaySegmentConnectionsModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& idx) const override;

    void setSegment(db_id segmentId, db_id fromGateId, db_id toGateId, bool reversed);
    void resetData();
    void clear();

    inline int getActualCount() const { return actualCount; }
    inline void setReadOnly(bool val) { readOnly = val; }
    inline bool isReadOnly() const  { return readOnly; }

    inline int getFromGateTrackCount() const { return m_fromGateTrackCount; }
    inline int getToGateTrackCount() const { return m_toGateTrackCount; }

    void createDefaultConnections();

    void removeAtRow(int row);
    void addNewConnection(int *outRow);

    bool applyChanges(db_id overrideSegmentId);

signals:
    void modelError(const QString& msg);

private:
    int insertOrReplace(const RailwayTrack& newTrack);

private:
    sqlite3pp::database &mDb;

    QVector<RailwayTrack> items;

    db_id m_segmentId;
    db_id m_fromGateId;
    db_id m_toGateId;
    int m_fromGateTrackCount;
    int m_toGateTrackCount;
    int actualCount;
    int incompleteRowAdded;
    bool m_reversed;
    bool readOnly;
};

#endif // RAILWAYSEGMENTCONNECTIONSMODEL_H
