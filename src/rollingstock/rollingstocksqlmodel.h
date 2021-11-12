#ifndef ROLLINGSTOCKSQLMODEL_H
#define ROLLINGSTOCKSQLMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"
#include "utils/sqldelegate/IFKField.h"

#include "utils/types.h"

#include <QVector>

class RollingstockSQLModel : public IPagedItemModel, public IFKField
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    typedef enum {
        Model = 0,
        Number,
        Suffix,
        Owner,
        TypeCol,
        NCols
    } Columns;

    typedef struct RSItem_
    {
        db_id rsId;
        db_id modelId;
        db_id ownerId;
        QString modelName;
        QString modelSuffix;
        QString ownerName;
        int number;
        RsType type;
    } RSItem;

    RollingstockSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);
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

    // IFKField
    bool getFieldData(int row, int col, db_id &idOut, QString &nameOut) const override;
    bool validateData(int row, int col, db_id id, const QString &name) override;
    bool setFieldData(int row, int col, db_id id, const QString &name) override;

    // RollingstockSQLModel
    bool removeRSItem(db_id rsId, const RSItem *item = nullptr);
    bool removeRSItemAt(int row);
    db_id addRSItem(int *outRow, QString *errOut = nullptr);

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Invalid

        const RSItem& item = cache.at(row - cacheFirstRow);
        return item.rsId;
    }

    bool removeAllRS();

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);
    void handleResult(const QVector<RSItem> &items, int firstRow);

    bool setModel(RSItem &item, db_id modelId, const QString &name);
    bool setOwner(RSItem &item, db_id ownerId, const QString &name);
    bool setNumber(RSItem &item, int number);

private:
    QVector<RSItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // ROLLINGSTOCKSQLMODEL_H
