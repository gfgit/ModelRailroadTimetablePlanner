#ifndef STATIONSMATCHMODEL_H
#define STATIONSMATCHMODEL_H

#include "utils/sqldelegate/isqlfkmatchmodel.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class StationsMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT
public:
    StationsMatchModel(database &db, QObject *parent = nullptr);

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString& text) override;
    virtual void refreshData() override;
    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

    // StationsMatchModel:
    void setFilter(db_id exceptStId);

private:
    struct StationItem
    {
        db_id stationId;
        QString name;
        char padding[4];
    };
    StationItem items[MaxMatchItems];

    database &mDb;
    query q_getMatches;

    db_id m_exceptStId;

    QByteArray mQuery;
};

#endif // STATIONSMATCHMODEL_H
