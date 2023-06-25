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

#ifndef RAILWAYSEGMENTSMODEL_H
#define RAILWAYSEGMENTSMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"
#include "stations/station_utils.h"

#include <QFlags>

struct RailwaySegmentsModelItem
{
    db_id segmentId;
    db_id fromStationId;
    db_id fromGateId;
    db_id toStationId;
    db_id toGateId;
    int maxSpeedKmH;
    int distanceMeters;
    QFlags<utils::RailwaySegmentType> type;

    QString fromStationName;
    QString toStationName;
    QChar fromGateLetter;
    QChar toGateLetter;
    QString segmentName;
    bool reversed;
};

class RailwaySegmentsModel
    : public IPagedItemModelImpl<RailwaySegmentsModel, RailwaySegmentsModelItem>
{
    Q_OBJECT

public:
    enum
    {
        BatchSize = 100
    };

    enum Columns
    {
        NameCol        = 0,
        FromStationCol = 1,
        FromGateCol,
        ToStationCol,
        ToGateCol,
        MaxSpeedCol,
        DistanceCol,
        IsElectrifiedCol,
        NCols
    };

    typedef RailwaySegmentsModelItem RailwaySegmentItem;
    typedef IPagedItemModelImpl<RailwaySegmentsModel, RailwaySegmentsModelItem> BaseClass;

    RailwaySegmentsModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &idx) const override;

    // IPagedItemModel

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) override;

    // RailwaySegmentsModel
    db_id getFilterFromStationId() const;
    void setFilterFromStationId(const db_id &value);

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; // Invalid

        const RailwaySegmentItem &item = cache.at(row - cacheFirstRow);
        return item.segmentId;
    }

    inline QString getNameAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return QString(); // Invalid

        const RailwaySegmentItem &item = cache.at(row - cacheFirstRow);
        return item.segmentName;
    }

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    friend BaseClass;
    Q_INVOKABLE void internalFetch(int firstRow, int sortCol, int valRow, const QVariant &val);

private:
    db_id filterFromStationId;
};

#endif // RAILWAYSEGMENTSMODEL_H
