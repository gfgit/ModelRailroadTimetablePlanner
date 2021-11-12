#ifndef STATIONSMODEL_H
#define STATIONSMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "utils/types.h"
#include "stations/station_utils.h"

#include <QVector>

//TODO: emit signals
class StationsModel : public IPagedItemModel
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    typedef enum {
        NameCol = 0,
        ShortNameCol,
        TypeCol,
        PhoneCol,
        NCols
    } Columns;

    typedef struct StationItem_
    {
        db_id stationId;
        qint64 phone_number;
        QString name;
        QString shortName;
        utils::StationType type;
    } StationItem;

    StationsModel(sqlite3pp::database &db, QObject *parent = nullptr);

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

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) override;

    // StationsModel
    bool addStation(const QString& name, db_id *outStationId = nullptr);
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

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int firstRow, int sortCol, int valRow, const QVariant &val);
    void handleResult(const QVector<StationItem> &items, int firstRow);

    bool setName(StationItem &item, const QString &val);
    bool setShortName(StationItem &item, const QString &val);
    bool setType(StationItem &item, int val);
    bool setPhoneNumber(StationsModel::StationItem &item, qint64 val);

private:
    QVector<StationItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // STATIONSMODEL_H
