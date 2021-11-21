#ifndef RAILWAYSEGMENTSMODEL_H
#define RAILWAYSEGMENTSMODEL_H

#include "utils/sqldelegate/pageditemmodelhelper.h"

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

class RailwaySegmentsModel : public IPagedItemModelImpl<RailwaySegmentsModel, RailwaySegmentsModelItem>
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    enum Columns {
        NameCol = 0,
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

    // RailwaySegmentsModel
    db_id getFilterFromStationId() const;
    void setFilterFromStationId(const db_id &value);

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Invalid

        const RailwaySegmentItem& item = cache.at(row - cacheFirstRow);
        return item.segmentId;
    }

    inline QString getNameAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return QString(); //Invalid

        const RailwaySegmentItem& item = cache.at(row - cacheFirstRow);
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
