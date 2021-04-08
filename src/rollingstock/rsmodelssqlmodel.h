#ifndef RSMODELSSQLMODEL_H
#define RSMODELSSQLMODEL_H

#include "utils/sqldelegate/pageditemmodel.h"

#include "utils/types.h"

#include <QVector>

class RSModelsSQLModel : public IPagedItemModel
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    typedef enum {
        Name = 0,
        Suffix,
        MaxSpeed,
        Axes,
        TypeCol,
        SubTypeCol,
        NCols
    } Columns;

    typedef struct RSModel_
    {
        db_id modelId;
        QString name;
        QString suffix;
        qint16 maxSpeedKmH;
        qint8 axes;
        RsType type;
        RsEngineSubType sub_type;
    } RSModel;

    RSModelsSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);
    bool event(QEvent *e) override;

    // QAbstractTableModel:

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


    // IPagedItemModel:

    // Cached rows management
    virtual void clearCache() override;

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) override;

    // RSModelsSQLModel:
    bool removeRSModel(db_id modelId, const QString &name);
    bool removeRSModelAt(int row);
    db_id addRSModel(int *outRow, db_id sourceModelId, const QString &suffix, QString *errOut);

    db_id getModelIdAtRow(int row) const;

    bool removeAllRSModels();

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);
    void handleResult(const QVector<RSModel> &items, int firstRow);

    bool setNameOrSuffix(RSModel &item, const QString &newName, bool suffix);
    bool setType(RSModel &item, RsType type, RsEngineSubType subType);

private:
    QVector<RSModel> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // RSMODELSSQLMODEL_H
