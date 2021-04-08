#ifndef STATIONTRACKCONNECTIONSMODEL_H
#define STATIONTRACKCONNECTIONSMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"
#include "utils/sqldelegate/IFKField.h"

#include "stations/station_utils.h"

class StationTrackConnectionsModel : public IPagedItemModel, public IFKField
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    typedef enum {
        TrackCol = 0,
        TrackSideCol,
        GateCol,
        GateTrackCol,
        NCols
    } Columns;

    typedef struct TrackConnItem_
    {
        db_id connId;
        db_id trackId;
        db_id gateId;
        QString trackName;
        QString gateName;
        int gateTrack;
        utils::Side trackSide;
    } TrackConnItem;

    StationTrackConnectionsModel(sqlite3pp::database &db, QObject *parent = nullptr);

    bool event(QEvent *e) override;

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
    virtual void refreshData(bool forceUpdate = false) override;

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

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortCol, int valRow, const QVariant &val);
    void handleResult(const QVector<TrackConnItem> &items, int firstRow);

    bool setTrackSide(TrackConnItem &item, int val);
    bool setGateTrack(TrackConnItem &item, int track);
    bool setTrack(TrackConnItem &item, db_id trackId, const QString &trackName);
    bool setGate(TrackConnItem &item, db_id gateId, const QString &gateName);

private:
    db_id m_stationId;
    QVector<TrackConnItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
    bool editable;
};

#endif // STATIONTRACKCONNECTIONSMODEL_H
