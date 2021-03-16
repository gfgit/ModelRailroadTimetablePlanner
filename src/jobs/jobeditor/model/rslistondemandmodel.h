#ifndef RSLISTONDEMANDMODEL_H
#define RSLISTONDEMANDMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "utils/types.h"

#include <QVector>


class RSListOnDemandModel : public IPagedItemModel
{
    Q_OBJECT

public:

    enum { BatchSize = 25 };

    typedef enum {
        Name = 0,
        NCols
    } Columns;

    typedef struct RSItem_
    {
        db_id rsId;
        QString name;
        RsType type;
    } RSItem;

    RSListOnDemandModel(sqlite3pp::database &db, QObject *parent = nullptr);

    bool event(QEvent *e) override;

    // QAbstractTableModel

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // IPagedItemModel

    // Cached rows management
    virtual void clearCache() override;
    virtual void setSortingColumn(int col) override;

private:
    virtual void internalFetch(int first, int sortColumn, int valRow, const QVariant &val) = 0;
    void fetchRow(int row);
    void handleResult(const QVector<RSItem> &items, int firstRow);

private:
    QVector<RSItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // RSLISTONDEMANDMODEL_H
