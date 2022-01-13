#ifndef ROLLINGSTOCKSQLMODEL_H
#define ROLLINGSTOCKSQLMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"
#include "utils/delegates/sql/IFKField.h"

#include "utils/types.h"

namespace sqlite3pp {
class query;
}

struct RollingstockSQLModelItem
{
    db_id rsId;
    db_id modelId;
    db_id ownerId;
    QString modelName;
    QString modelSuffix;
    QString ownerName;
    int number;
    RsType type;
};

class RollingstockSQLModel : public IPagedItemModelImpl<RollingstockSQLModel, RollingstockSQLModelItem>,
                             public IFKField
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    enum Columns {
        Model = 0,
        Number,
        Suffix,
        Owner,
        TypeCol,
        NCols
    };

    typedef RollingstockSQLModelItem RSItem;
    typedef IPagedItemModelImpl<RollingstockSQLModel, RollingstockSQLModelItem> BaseClass;

    RollingstockSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& idx) const override;


    // IPagedItemModel

    virtual void setSortingColumn(int col) override;

    //Filter
    std::pair<QString, FilterFlags> getFilterAtCol(int col) override;
    bool setFilterAtCol(int col, const QString& str) override;

    // IFKField
    bool getFieldData(int row, int col, db_id &idOut, QString &nameOut) const override;
    bool validateData(int row, int col, db_id id, const QString &name) override;
    bool setFieldData(int row, int col, db_id id, const QString &name) override;

    // RollingstockSQLModel
    bool removeRSItem(db_id rsId, const RSItem *item = nullptr);
    bool removeRSItemAt(int row);
    db_id addRSItem(int *outRow, QString *errOut = nullptr);

    bool removeAllRS();

    // Convinience
    inline db_id getIdAtRow(int row) const
    {
        if (row < cacheFirstRow || row >= cacheFirstRow + cache.size())
            return 0; //Invalid

        const RSItem& item = cache.at(row - cacheFirstRow);
        return item.rsId;
    }


protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    friend BaseClass;
    void buildQuery(sqlite3pp::query &q, int sortCol, int offset, bool fullData);
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);

    bool setModel(RSItem &item, db_id modelId, const QString &name);
    bool setOwner(RSItem &item, db_id ownerId, const QString &name);
    bool setNumber(RSItem &item, int number);


private:
    QString m_modelFilter;
    QString m_numberFilter;
    QString m_ownerFilter;
};

#endif // ROLLINGSTOCKSQLMODEL_H
