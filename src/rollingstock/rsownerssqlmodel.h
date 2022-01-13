#ifndef RSOWNERSSQLMODEL_H
#define RSOWNERSSQLMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"

struct RSOwnersSQLModelItem
{
    db_id ownerId;
    QString name;
};


class RSOwnersSQLModel : public IPagedItemModelImpl<RSOwnersSQLModel, RSOwnersSQLModelItem>
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    enum Columns {
        Name = 0,
        NCols
    };

    typedef RSOwnersSQLModelItem RSOwner;
    typedef IPagedItemModelImpl<RSOwnersSQLModel, RSOwnersSQLModelItem> BaseClass;

    RSOwnersSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);

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

    // RSOwnersSQLModel

    bool removeRSOwner(db_id ownerId, const QString &name);
    bool removeRSOwnerAt(int row);
    db_id addRSOwner(int *outRow);

    bool removeAllRSOwners();

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    friend BaseClass;
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);

private:
    QString m_ownerFilter;
};

#endif // RSOWNERSSQLMODEL_H
