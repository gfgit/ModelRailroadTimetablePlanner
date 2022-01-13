#ifndef RSLISTONDEMANDMODEL_H
#define RSLISTONDEMANDMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"

#include <QVector>

struct RSListOnDemandModelItem
{
    db_id rsId;
    QString name;
    RsType type;
};

class RSListOnDemandModel : public IPagedItemModelImpl<RSListOnDemandModel, RSListOnDemandModelItem>
{
    Q_OBJECT

public:
    enum { BatchSize = 50 };

    enum Columns {
        Name = 0,
        NCols
    };

    typedef RSListOnDemandModelItem RSItem;
    typedef IPagedItemModelImpl<RSListOnDemandModel, RSListOnDemandModelItem> BaseClass;

    RSListOnDemandModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

private:
    friend BaseClass;
    virtual void internalFetch(int first, int sortColumn, int valRow, const QVariant &val) = 0;
};

#endif // RSLISTONDEMANDMODEL_H
