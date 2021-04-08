#ifndef STATIONLINESLISTMODEL_H
#define STATIONLINESLISTMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "utils/types.h"

#include <QVector>

//FIXME remove and use instad RailwaySegmentMatchModel
class StationLinesListModel : public IPagedItemModel
{
    Q_OBJECT

public:

    enum { BatchSize = 25 };

    typedef enum {
        Name = 0,
        NCols
    } Columns;

    typedef struct RSOwner_
    {
        db_id lineId;
        QString name;
    } LineItem;

    StationLinesListModel(sqlite3pp::database &db, QObject *parent = nullptr);
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

    // StationLinesListModel
    void setStationId(db_id stationId);

    int getLineRow(db_id lineId);
    db_id getLineIdAt(int row);

signals:
    void resultsReady();

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);
    void handleResult(const QVector<LineItem> &items, int firstRow);

private:
    db_id m_stationId;
    QVector<LineItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // STATIONLINESLISTMODEL_H
