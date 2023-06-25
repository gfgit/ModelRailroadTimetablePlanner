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

#ifndef LINESEGMENTSMODEL_H
#define LINESEGMENTSMODEL_H

#include "utils/delegates/sql/pageditemmodel.h"

#include <QVector>

#include <QFlags>

#include "utils/types.h"

#include "stations/station_utils.h"

/* NOTE: LineSegmentsModel should be loaded in signle batch
 *
 * Loading on demand has problems:
 * - You must be careful on row counting like exclude/include last row which is a station without
 * next segment
 * - You must tell the user that there is a page after the current one otherwise he may think the
 * line ends there.
 * - You cannot easily calculate the position of first page item
 *   (You must always start from first segment which may be on a previous page)
 *
 * We could LIMIT to a fixed maximum (e.g. 100)
 * And prevent creation of longer lines already in database
 * IMPLENENTATION:
 * CHECK(pos<100) and UNIQUE(id, pos) on 'lines' table
 * This ensures line have at maximum 100 segments (So 101 stations)
 */

static constexpr const int MaxSegmentsPerLine = 100;

class LineSegmentsModel : public IPagedItemModel
{
    Q_OBJECT

public:
    enum
    {
        BatchSize = MaxSegmentsPerLine
    };

    enum Columns
    {
        SegmentPosCol           = -1, // Invisible (Vertical header)
        StationOrSegmentNameCol = 0,
        KmPosCol,
        MaxSpeedCol,
        DistanceCol,
        NCols
    };

    struct LineSegmentItem
    {
        db_id lineSegmentId;
        db_id railwaySegmentId;
        db_id fromStationId;
        QString segmentName;
        QString fromStationName;
        int distanceMeters;
        int fromPosMeters;
        int maxSpeedKmH;
        QFlags<utils::RailwaySegmentType> segmentType;
        bool reversed;
    };

    LineSegmentsModel(sqlite3pp::database &db, QObject *parent = nullptr);

    bool event(QEvent *e) override;

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // IPagedItemModel

    // Cached rows management
    // NOTE: custom implementation
    virtual void clearCache() override;
    virtual void refreshData(bool forceUpdate = false) override;

    // LineSegmentsModel
    void setLine(db_id lineId);

    bool getLineInfo(QString &nameOut, int &startMetersOut) const;
    bool setLineInfo(const QString &name, int startMeters);

    inline int getSegmentCount() const
    {
        return curItemCount;
    }
    inline db_id getLastStation() const
    {
        // Use fake last segment which contains last station
        if (segments.size() < 1)
            return 0;
        return segments.last().fromStationId;
    }
    inline db_id getLastRailwaySegment() const
    {
        // Use ONE BUT LAST segment which contains last railway segment
        if (segments.size() < 2)
            return 0;
        return segments[segments.size() - 2].railwaySegmentId;
    }
    inline QString getStationNameAt(int itemPos) const
    {
        // Use ONE BUT LAST segment which contains last railway segment
        if (itemPos >= segments.size())
            return QString();
        return segments[itemPos].fromStationName;
    }

    bool removeSegmentsAfterPosInclusive(int pos);
    bool addStation(db_id railwaySegmentId, bool reverse);

    // Row Type
    // Event: stations
    // Odd:   segments
    enum RowType
    {
        StationRow,
        SegmentRow
    };

    inline RowType getRowType(int row) const
    {
        return (row % 2) == 0 ? StationRow : SegmentRow;
    }
    inline int getItemIndex(int row) const
    {
        return (row - (row % 2)) / 2;
    }

private:
    void fetchRows();

private:
    QVector<LineSegmentItem> segments;
    db_id m_lineId;
    bool isFetching;
};

#endif // LINESEGMENTSMODEL_H
