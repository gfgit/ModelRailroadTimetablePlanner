#ifndef SHIFTSQLMODEL_H
#define SHIFTSQLMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "utils/types.h"

#include <QVector>

#include "shiftitem.h"

//FIXME: remove old shift model
class ShiftSQLModel : public IPagedItemModel
{
    Q_OBJECT

public:
    enum Columns
    {
        ShiftName = 0,
        NCols
    };

    enum { BatchSize = 100 };

    ShiftSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);
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

    // ShiftSQLModel
    db_id shiftAtRow(int row) const;
    QString shiftNameAtRow(int row) const;

    bool removeShift(db_id shiftId);
    bool removeShiftAt(int row);

    db_id addShift(int *outRow);

public slots:
    void setQuery(const QString& text);

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int valRow, const QString &val);
    void handleResult(const QVector<ShiftItem> items, int firstRow);

private:
    QVector<ShiftItem> cache;

    QString mQuery;

    int cacheFirstRow;
    int firstPendingRow;
};

#endif // SHIFTSQLMODEL_H
