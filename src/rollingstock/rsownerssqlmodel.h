#ifndef RSOWNERSSQLMODEL_H
#define RSOWNERSSQLMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "utils/types.h"

#include <QVector>


class RSOwnersSQLModel : public IPagedItemModel
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    typedef enum {
        Name = 0,
        NCols
    } Columns;

    typedef struct RSOwner_
    {
        db_id ownerId;
        QString name;
    } RSOwner;

    RSOwnersSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);
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

    // RSOwnersSQLModel

    bool removeRSOwner(db_id ownerId, const QString &name);
    bool removeRSOwnerAt(int row);
    db_id addRSOwner(int *outRow);

    bool removeAllRSOwners();

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);
    void handleResult(const QVector<RSOwner> &items, int firstRow);

private:
    QVector<RSOwner> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // RSOWNERSSQLMODEL_H
