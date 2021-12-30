#ifndef RSMODELSSQLMODEL_H
#define RSMODELSSQLMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"

struct RSModelsSQLModelItem
{
    db_id modelId;
    QString name;
    QString suffix;
    qint16 maxSpeedKmH;
    qint8 axes;
    RsType type;
    RsEngineSubType sub_type;
};


class RSModelsSQLModel : public IPagedItemModelImpl<RSModelsSQLModel, RSModelsSQLModelItem>
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    enum Columns {
        Name = 0,
        Suffix,
        MaxSpeed,
        Axes,
        TypeCol,
        SubTypeCol,
        NCols
    };

    typedef RSModelsSQLModelItem RSModel;
    typedef IPagedItemModelImpl<RSModelsSQLModel, RSModelsSQLModelItem> BaseClass;

    RSModelsSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel:

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& idx) const override;


    // IPagedItemModel:

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
    friend BaseClass;
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);

    bool setNameOrSuffix(RSModel &item, const QString &newName, bool suffix);
    bool setType(RSModel &item, RsType type, RsEngineSubType subType);
};

#endif // RSMODELSSQLMODEL_H
