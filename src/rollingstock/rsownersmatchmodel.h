#ifndef RSOWNERSMATCHMODEL_H
#define RSOWNERSMATCHMODEL_H

#include "utils/sqldelegate/isqlfkmatchmodel.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class RSOwnersMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    RSOwnersMatchModel(database &db, QObject *parent = nullptr);

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString& text) override;
    virtual void refreshData() override;
    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

private:
    struct RSOwnerItem
    {
        db_id ownerId;
        QString name;
        char padding[4];
    };
    RSOwnerItem items[MaxMatchItems];

    database &mDb;
    query q_getMatches;
    QByteArray mQuery;
};

#endif // RSOWNERSMATCHMODEL_H
