#ifndef STATIONORLINEMATCHFACTORY_H
#define STATIONORLINEMATCHFACTORY_H

#include "utils/sqldelegate/imatchmodelfactory.h"
#include "railwaynodemode.h"

namespace sqlite3pp
{
class database;
}

//FIXME: remove
class StationOrLineMatchFactory : public IMatchModelFactory
{
public:
    StationOrLineMatchFactory(sqlite3pp::database &db, QObject *parent);

    ISqlFKMatchModel *createModel() override;

    inline void setMode(RailwayNodeMode mode) { m_mode = mode; }

private:
    sqlite3pp::database &mDb;
    RailwayNodeMode m_mode;
};

#endif // STATIONORLINEMATCHFACTORY_H
