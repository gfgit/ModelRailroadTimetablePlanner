#ifndef STATIONGATESMATCHMODEL_H
#define STATIONGATESMATCHMODEL_H

#include "utils/sqldelegate/isqlfkmatchmodel.h"
#include "utils/sqldelegate/imatchmodelfactory.h"

#include "utils/types.h"
#include "stations/station_utils.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QFlags>

class StationGatesMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    StationGatesMatchModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString& text) override;
    virtual void refreshData() override;
    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

    // StationsMatchModel:
    void setFilter(db_id stationId, bool hideAlreadyConnected);

private:
    struct GateItem
    {
        db_id gateId;
        QString name;
        QFlags<utils::GateType> type;
        utils::Side side;
    };
    static const int ItemCount = 30;
    GateItem items[ItemCount];

    sqlite3pp::database &mDb;
    sqlite3pp::query q_getMatches;

    db_id m_stationId;
    bool hideConnectedGates;
    QByteArray mQuery;
};

class StationGatesMatchFactory : public IMatchModelFactory
{
public:
    StationGatesMatchFactory(sqlite3pp::database &db, QObject *parent);

    virtual ISqlFKMatchModel *createModel() override;

    inline void setStationId(db_id stationId) { m_stationId = stationId; }
    inline void setHideConnectedGates(bool val) { hideConnectedGates = val; }

private:
    db_id m_stationId;
    bool hideConnectedGates;
    sqlite3pp::database &mDb;
};

#endif // STATIONGATESMATCHMODEL_H
