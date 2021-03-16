#ifndef RAILWAYNODEMODEL_H
#define RAILWAYNODEMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"
#include "utils/sqldelegate/IFKField.h"

#include "utils/types.h"
#include "utils/directiontype.h"
#include "railwaynodemode.h"

class RailwayNodeModel : public IPagedItemModel, public IFKField
{
    Q_OBJECT
public:

    enum { BatchSize = 100 };

    typedef enum {
        LineOrStationCol = 0,
        KmCol,
        DirectionCol,
        NCols
    } Columns;

    class Item
    {
    public:
        db_id nodeId;
        db_id lineOrStationId;
        QString lineOrStationName;
        int kmInMeters;
        Direction direction;
    };

    RailwayNodeModel(sqlite3pp::database &db, QObject *parent = nullptr);

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

    // IFKField
    bool getFieldData(int row, int col, db_id &idOut, QString &nameOut) const override;
    bool validateData(int row, int col, db_id id, const QString &name) override;
    bool setFieldData(int row, int col, db_id id, const QString &name) override;

    // RailwayNodeModel
    inline RailwayNodeMode getMode() const { return m_mode; }

    void setMode(db_id id, RailwayNodeMode mode);
    db_id addItem(db_id lineOrStationId, int *outRow);
    bool removeItem(int row);

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);
    void handleResult(const QVector<Item> &items, int firstRow);

    bool setLineOrStation(Item &item, db_id id, const QString &name);
    bool setKm(Item &item, int kmInMeters);
    bool setDirection(Item &item, Direction dir);

    int getLineMaxKmInMeters(db_id lineId);
    bool setNodeKm(db_id nodeId, int kmInMeters);

private:
    QVector<Item> cache;
    int cacheFirstRow;
    int firstPendingRow;

    db_id stationOrLineId;
    RailwayNodeMode m_mode;
};

#endif // RAILWAYNODEMODEL_H
