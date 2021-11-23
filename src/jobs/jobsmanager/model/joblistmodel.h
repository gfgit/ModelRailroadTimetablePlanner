#ifndef JOBLISTMODEL_H
#define JOBLISTMODEL_H

#include "utils/sqldelegate/pageditemmodelhelper.h"

#include "utils/types.h"

#include <QTime>

struct JobListModelItem
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
};

class JobListModel : public IPagedItemModelImpl<JobListModel, JobListModelItem>
{
    Q_OBJECT
public:
    enum { BatchSize = 100 };

    enum Columns {
        IdCol = 0,
        Category,
        ShiftCol,
        OriginSt,
        OriginTime,
        DestinationSt,
        DestinationTime,
        NCols
    };

    typedef JobListModelItem JobItem;
    typedef IPagedItemModelImpl<JobListModel, JobListModelItem> BaseClass;

    JobListModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // IPagedItemModel

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
    friend BaseClass;
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);
};

#endif // JOBLISTMODEL_H
