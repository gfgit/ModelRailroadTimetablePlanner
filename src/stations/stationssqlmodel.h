#ifndef STATIONSSQLMODEL_H
#define STATIONSSQLMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "utils/types.h"

#include <QVector>

#include <QRgb>

class StationsSQLModel : public IPagedItemModel
{
    Q_OBJECT
public:

    enum { BatchSize = 100 };

    typedef enum {
        NameCol = 0,
        ShortName,
        Platforms,
        Depots,
        PlatformColor,
        DefaultFreightPlatf,
        DefaultPassengerPlatf,
        NCols
    } Columns;

    typedef struct StationItem_
    {
        db_id stationId;
        QString name;
        QString shortName;
        QRgb platfColor;

        qint8 platfs;
        qint8 depots;
        qint8 defaultFreightPlatf;
        qint8 defaultPassengerPlatf;
    } StationItem;

    StationsSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);

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

    // StationsSQLModel
    bool addStation(db_id *outStationId = nullptr);
    bool removeStation(db_id stationId);

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Invalid

        const StationItem& item = cache.at(row - cacheFirstRow);
        return item.stationId;
    }

    inline QString getNameAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return QString(); //Invalid

        const StationItem& item = cache.at(row - cacheFirstRow);
        return item.name;
    }

    inline QPair<int, int> getPlatfCountAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return {-1, -1}; //Invalid

        const StationItem& item = cache.at(row - cacheFirstRow);
        return {item.platfs, item.depots};
    }

private slots:
    void onStationAdded();
    void onStationRemoved();

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortCol, int valRow, const QVariant &val);
    void handleResult(const QVector<StationItem> &items, int firstRow);

    bool setName(StationItem &item, const QString &val);
    bool setShortName(StationItem &item, const QString &val);
    bool setPlatfCount(StationItem &item, int platf);
    bool setDepotCount(StationItem &item, int depots);
    bool setDefaultFreightPlatf(StationItem &item, int platf);
    bool setDefaultPassengerPlatf(StationItem &item, int platf);

private:
    QVector<StationItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // STATIONSSQLMODEL_H
