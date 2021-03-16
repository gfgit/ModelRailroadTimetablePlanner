#ifndef LINESSQLMODEL_H
#define LINESSQLMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "utils/types.h"

#include <QVector>

class LinesSQLModel : public IPagedItemModel
{
    Q_OBJECT
public:

    enum { BatchSize = 100 };

    typedef enum {
        LineNameCol = 0,
        LineMaxSpeedKmHCol,
        LineTypeCol,
        NCols
    } Columns;

    typedef struct LineItem_
    {
        db_id lineId;
        QString name;
        int maxSpeedKmH;
        LineType type;
    } LineItem;

    LinesSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);
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
    virtual void refreshData() override;

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) override;

    // LinesSQLModel
    bool removeLine(db_id lineId);
    db_id addLine(int *outRow, QString *errOut = nullptr);

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

private slots:
    void onLineAdded();
    void onLineRemoved();

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);
    void handleResult(const QVector<LineItem> &items, int firstRow);

    bool setName(LineItem &item, const QString &name);
    bool setMaxSpeed(LineItem &item, int speed);
    bool setType(LineItem &item, LineType type);

private:
    QVector<LineItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // LINESSQLMODEL_H
