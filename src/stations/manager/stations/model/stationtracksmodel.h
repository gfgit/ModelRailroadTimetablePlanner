#ifndef STATIONTRACKSMODEL_H
#define STATIONTRACKSMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "stations/station_utils.h"

#include <QRgb>

#include <QFlags>

class StationTracksModel : public IPagedItemModel
{
    Q_OBJECT

public:
    enum { BatchSize = 100 };

    typedef enum {
        PosCol = -1, //Fake column, uses row header
        NameCol = 0,
        ColorCol,
        IsElectrifiedCol,
        IsThroughCol,
        TrackLengthCol,
        PassengerLegthCol,
        FreightLengthCol,
        MaxAxesCol,
        NCols
    } Columns;

    typedef struct TrackItem_
    {
        db_id trackId;
        int trackLegth;
        int platfLength;
        int freightLength;
        int maxAxesCount;
        QString name;
        QRgb color;
        QFlags<utils::StationTrackType> type;
    } TrackItem;

    StationTracksModel(sqlite3pp::database &db, QObject *parent = nullptr);

    bool event(QEvent *e) override;

    // QAbstractTableModel

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

    // IPagedItemModel

    // Cached rows management
    virtual void clearCache() override;
    virtual void refreshData() override;

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) override;

    // StationTracksModel

    bool setStation(db_id stationId);
    inline db_id getStation() const { return m_stationId; }

    bool addTrack(int pos, const QString& name, db_id *outTrackId = nullptr);
    bool removeTrack(db_id trackId);
    bool moveTrackUpDown(db_id trackId, bool up, bool topOrBottom);

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Invalid

        const TrackItem& item = cache.at(row - cacheFirstRow);
        return item.trackId;
    }

    inline QString getNameAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return QChar(); //Invalid

        const TrackItem& item = cache.at(row - cacheFirstRow);
        return item.name;
    }

    inline bool isEditable() const { return editable; }
    inline void setEditable(bool val) { editable = val; }

    inline bool hasAtLeastOneTrack()
    {
        refreshData(); //Recalc count
        return getTotalItemsCount() > 0;
    }

signals:
    void trackNameChanged(db_id trackId, const QString& name);
    void trackRemoved(db_id trackId);

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortCol, int valRow, const QVariant &val);
    void handleResult(const QVector<TrackItem> &items, int firstRow);

    bool setName(TrackItem &item, const QString &name);
    bool setType(TrackItem &item, QFlags<utils::StationTrackType> type);
    bool setLength(TrackItem &item, int val, Columns column);
    bool setColor(StationTracksModel::TrackItem &item, QRgb color);
    void moveTrack(db_id trackId, int pos);

private:
    db_id m_stationId;
    QVector<TrackItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
    bool editable;
};

#endif // STATIONTRACKSMODEL_H
