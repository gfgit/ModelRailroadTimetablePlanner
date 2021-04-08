#ifndef LINESMODEL_H
#define LINESMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "utils/types.h"

#include <QVector>

//TODO: emit signals
class LinesModel : public IPagedItemModel
{
    Q_OBJECT

public:
    enum { BatchSize = 100 };

    typedef enum {
        NameCol = 0,
        StartKm,
        NCols
    } Columns;

    typedef struct LineItem_
    {
        db_id lineId;
        QString name;
        int startMeters;
    } LineItem;

    LinesModel(sqlite3pp::database &db, QObject *parent = nullptr);

    bool event(QEvent *e) override;

    // QAbstractTableModel

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // IPagedItemModel

    // Cached rows management
    virtual void clearCache() override;

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) override;

    // LinesModel
    bool addLine(const QString& name, db_id *outLineId = nullptr);
    bool removeLine(db_id lineId);

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Invalid

        const LineItem& item = cache.at(row - cacheFirstRow);
        return item.lineId;
    }

    inline QString getNameAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return QString(); //Invalid

        const LineItem& item = cache.at(row - cacheFirstRow);
        return item.name;
    }

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortCol, int valRow, const QVariant &val);
    void handleResult(const QVector<LineItem> &items, int firstRow);

private:
    QVector<LineItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // LINESMODEL_H
