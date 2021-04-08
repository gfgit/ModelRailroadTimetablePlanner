#ifndef LINESEGMENTSMODEL_H
#define LINESEGMENTSMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include <QVector>

#include <QFlags>

#include "utils/types.h"

#include "stations/station_utils.h"

/* NOTE: LineSegmentsModel should be loaded in signle batch
 *
 * Loading on demand has problems:
 * - You must be careful on row counting like exclude/include last row which is a station without next segment
 * - You must tell the user that there is a page after the current one otherwise he may think the line ends there.
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

    enum { BatchSize = MaxSegmentsPerLine };

    typedef enum {
        SegmentPosCol = -1, //Invisible (Vertical header)
        StationOrSegmentNameCol = 0,
        KmPosCol,
        MaxSpeedCol,
        DistanceCol,
        NCols
    } Columns;

    typedef struct LineSegmentItem_
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
    } LineSegmentItem;

    LineSegmentsModel(sqlite3pp::database &db, QObject *parent = nullptr);

    bool event(QEvent *e) override;

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // IPagedItemModel

    // Cached rows management
    //NOTE: custom implementation
    virtual void clearCache() override;
    virtual void refreshData(bool forceUpdate = false) override;

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) override;

    // LineSegmentsModel
    void setLine(db_id lineId);

    bool getLineInfo(QString &nameOut, int &startMetersOut) const;
    bool setLineInfo(const QString &name, int startMeters);

    inline int getSegmentCount() const { return curItemCount; }
    inline db_id getLastStation() const
    {
        //Use fake last segment which contains last station
        if(segments.size() < 1)
            return 0;
        return segments.last().fromStationId;
    }
    inline db_id getLastRailwaySegment() const
    {
        //Use ONE BUT LAST segment which contains last railway segment
        if(segments.size() < 2)
            return 0;
        return segments[segments.size() - 2].railwaySegmentId;
    }
    inline QString getStationNameAt(int itemPos) const
    {
        //Use ONE BUT LAST segment which contains last railway segment
        if(itemPos >= segments.size())
            return QString();
        return segments[itemPos].fromStationName;
    }

    bool removeSegmentsAfterPosInclusive(int pos);
    bool addStation(db_id railwaySegmentId, bool reverse);

    //Row Type
    //Event: stations
    //Odd:   segments
    typedef enum  {
        StationRow,
        SegmentRow
    } RowType;

    inline RowType getRowType(int row) const { return (row % 2) == 0 ? StationRow : SegmentRow; }
    inline int getItemIndex(int row) const { return (row - (row % 2)) / 2; }

private:
    void fetchRows();

private:
    QVector<LineSegmentItem> segments;
    db_id m_lineId;
    bool isFetching;
};

#endif // LINESEGMENTSMODEL_H
