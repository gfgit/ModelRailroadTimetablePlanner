#ifndef SHIFTSQLMODEL_H
#define SHIFTSQLMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"

#include <QVector>

struct ShiftsModelItem
{
    db_id shiftId;
    QString shiftName;
};

/*!
 * \brief The ShiftsModel class
 *
 * Paged model to show Job Shifts
 */
class ShiftsModel : public IPagedItemModelImpl<ShiftsModel, ShiftsModelItem>
{
    Q_OBJECT

public:
    enum { BatchSize = 100 };

    enum Columns
    {
        ShiftName = 0,
        NCols
    };

    typedef ShiftsModelItem ShiftItem;
    typedef IPagedItemModelImpl<ShiftsModel, ShiftsModelItem> BaseClass;

    ShiftsModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& idx) const override;

    // IPagedItemModel

    //Filter
    std::pair<QString, FilterFlags> getFilterAtCol(int col) override;
    bool setFilterAtCol(int col, const QString& str) override;

    // ShiftModel

    bool removeShift(db_id shiftId);
    bool removeShiftAt(int row);

    db_id addShift(int *outRow);

    // Convinience
    inline db_id shiftAtRow(int row) const
    {
        if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Not fetched yet or invalid

        return cache.at(row - cacheFirstRow).shiftId;
    }

    inline QString shiftNameAtRow(int row) const
    {
        if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return QString(); //Not fetched yet or invalid

        return cache.at(row - cacheFirstRow).shiftName;
    }

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    friend BaseClass;
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);
    void handleResult(const QVector<ShiftItem> items, int firstRow);

private:
    QString m_nameFilter;
};

#endif // SHIFTSQLMODEL_H
