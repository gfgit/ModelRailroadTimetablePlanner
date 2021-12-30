#ifndef LINESMODEL_H
#define LINESMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"

struct LinesModelItem
{
    db_id lineId;
    QString name;
    int startMeters;
};

class LinesModel : public IPagedItemModelImpl<LinesModel, LinesModelItem>
{
    Q_OBJECT

public:
    enum { BatchSize = 100 };

    enum Columns {
        NameCol = 0,
        StartKm,
        NCols
    };

    typedef LinesModelItem LineItem;
    typedef IPagedItemModelImpl<LinesModel, LinesModelItem> BaseClass;

    LinesModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // IPagedItemModel

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
    friend BaseClass;
    Q_INVOKABLE void internalFetch(int first, int sortCol, int valRow, const QVariant &val);
};

#endif // LINESMODEL_H
