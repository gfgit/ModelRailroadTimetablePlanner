#ifndef RSMODELSSQLMODEL_H
#define RSMODELSSQLMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"

namespace sqlite3pp {
class query;
}

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

    virtual void setSortingColumn(int col) override;

    //Filter
    std::pair<QString, FilterFlags> getFilterAtCol(int col) override;
    bool setFilterAtCol(int col, const QString& str) override;

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
    void buildQuery(sqlite3pp::query &q, int sortCol, int offset, bool fullData);
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);

    bool setNameOrSuffix(RSModel &item, const QString &newName, bool suffix);
    bool setType(RSModel &item, RsType type, RsEngineSubType subType);

private:
    QString m_nameFilter;
    QString m_suffixFilter;
    QString m_speedFilter;
};

#endif // RSMODELSSQLMODEL_H
