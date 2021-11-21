#ifndef JOBLISTMODEL_H
#define JOBLISTMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "utils/types.h"

#include <QVector>

#include <QTime>

class JobListModel : public IPagedItemModel
{
    Q_OBJECT
public:
    enum { BatchSize = 100 };

    typedef enum {
        IdCol = 0,
        Category,
        ShiftCol,
        OriginSt,
        OriginTime,
        DestinationSt,
        DestinationTime,
        NCols
    } Columns;

    typedef struct JobItem_
    {
        db_id jobId;
        db_id shiftId;
        db_id originStId;
        db_id destStId;
        QTime originTime;
        QTime destTime;
        QString shiftName;
        QString origStName;
        QString destStName;
        JobCategory category;
    } JobItem;


    JobListModel(sqlite3pp::database &db, QObject *parent = nullptr);

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

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Invalid

        const JobItem& item = cache.at(row - cacheFirstRow);
        return item.jobId;
    }

    inline QPair<db_id, JobCategory> getShiftAnCatAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return {0, JobCategory::NCategories}; //Invalid

        const JobItem& item = cache.at(row - cacheFirstRow);
        return {item.shiftId, item.category};
    }

private slots:
    void onJobAddedOrRemoved();

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);
    void handleResult(const QVector<JobItem> &items, int firstRow);

private:
    QVector<JobItem> cache;

    int cacheFirstRow;
    int firstPendingRow;
};

#endif // JOBLISTMODEL_H
