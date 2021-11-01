#ifndef STATIONSMATCHMODEL_H
#define STATIONSMATCHMODEL_H

#include "utils/sqldelegate/isqlfkmatchmodel.h"
#include "utils/sqldelegate/imatchmodelfactory.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class StationsMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT
public:
    enum Columns
    {
        StationCol = 0,
        SegmentCol,
        NCols
    };

    StationsMatchModel(database &db, QObject *parent = nullptr);

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    int columnCount(const QModelIndex &p = QModelIndex()) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString& text) override;
    void refreshData() override;

    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

    db_id getSegmentAtRow(int row) const;

    // StationsMatchModel:
    void setFilter(db_id connectedToStationId, db_id exceptStId);

private:
    struct StationItem
    {
        db_id stationId = 0;
        db_id segmentId = 0;
        QString stationName;
        QString segmentName;
    };
    StationItem items[MaxMatchItems];

    database &mDb;
    query q_getMatches;

    db_id mFromStationId;
    db_id mExceptStId;

    QByteArray mQuery;
};

class StationMatchFactory : public IMatchModelFactory
{
    StationMatchFactory(sqlite3pp::database &db, QObject *parent);

    ISqlFKMatchModel *createModel() override;

    inline void setExceptStation(db_id fromStId, db_id exceptStId)
    {
        mFromStationId = fromStId;
        mExceptStId = exceptStId;
    }

private:
    db_id mFromStationId;
    db_id mExceptStId;
    sqlite3pp::database &mDb;
};

#endif // STATIONSMATCHMODEL_H
