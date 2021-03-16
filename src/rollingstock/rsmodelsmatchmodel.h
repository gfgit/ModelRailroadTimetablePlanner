#ifndef RSMODELSMATCHMODEL_H
#define RSMODELSMATCHMODEL_H

#include "utils/sqldelegate/isqlfkmatchmodel.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class RSModelsMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    RSModelsMatchModel(database &db, QObject *parent = nullptr);

    // QAbstractTableModel
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel
    void autoSuggest(const QString& text) override;
    virtual void refreshData() override;
    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

private:
    struct RSModelItem
    {
        db_id modelId;
        QString nameWithSuffix;
        int nameLength;
    };
    RSModelItem items[MaxMatchItems];

    database &mDb;
    query q_getMatches;
    QByteArray mQuery;
};

#endif // RSMODELSMATCHMODEL_H
