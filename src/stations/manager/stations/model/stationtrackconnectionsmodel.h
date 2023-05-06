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

#ifndef STATIONTRACKCONNECTIONSMODEL_H
#define STATIONTRACKCONNECTIONSMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"
#include "utils/delegates/sql/IFKField.h"

#include "stations/station_utils.h"

struct StationTrackConnectionsModelItem
{
    db_id connId;
    db_id trackId;
    db_id gateId;
    QString trackName;
    QString gateName;
    int gateTrack;
    utils::Side trackSide;
};

class StationTrackConnectionsModel : public IPagedItemModelImpl<StationTrackConnectionsModel, StationTrackConnectionsModelItem>,
                                     public IFKField
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    enum Columns{
        TrackCol = 0,
        TrackSideCol,
        GateCol,
        GateTrackCol,
        NCols
    };

    typedef StationTrackConnectionsModelItem TrackConnItem;
    typedef IPagedItemModelImpl<StationTrackConnectionsModel, StationTrackConnectionsModelItem> BaseClass;

    StationTrackConnectionsModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& idx) const override;

    // IPagedItemModel

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) override;

    // IFKField

    virtual bool getFieldData(int row, int col, db_id &idOut, QString& nameOut) const override;
    virtual bool validateData(int row, int col, db_id id, const QString& name) override;
    virtual bool setFieldData(int row, int col, db_id id, const QString& name) override;

    // StationTrackConnectionsModel

    bool setStation(db_id stationId);
    inline db_id getStation() const { return m_stationId; }

    bool addTrackConnection(db_id trackId, utils::Side trackSide,
                            db_id gateId, int gateTrack);
    bool removeTrackConnection(db_id connId);

    bool addTrackToAllGatesOnSide(db_id trackId, utils::Side side, int preferredGateTrack);
    bool addGateToAllTracks(db_id gateId, int gateTrack);

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Invalid

        const TrackConnItem& item = cache.at(row - cacheFirstRow);
        return item.connId;
    }

    inline bool isEditable() const { return editable; }
    inline void setEditable(bool val) { editable = val; }

signals:
    void trackConnRemoved(db_id connId, db_id trackId, db_id gateId);

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    friend BaseClass;
    Q_INVOKABLE void internalFetch(int first, int sortCol, int valRow, const QVariant &val);

    bool setTrackSide(TrackConnItem &item, int val);
    bool setGateTrack(TrackConnItem &item, int track);
    bool setTrack(TrackConnItem &item, db_id trackId, const QString &trackName);
    bool setGate(TrackConnItem &item, db_id gateId, const QString &gateName);

private:
    db_id m_stationId;
    bool editable;
};

#endif // STATIONTRACKCONNECTIONSMODEL_H
