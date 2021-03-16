#include "stationorlinematchfactory.h"

#include "stations/stationsmatchmodel.h"
#include "lines/linesmatchmodel.h"

StationOrLineMatchFactory::StationOrLineMatchFactory(sqlite3pp::database &db, QObject *parent) :
    IMatchModelFactory(parent),
    mDb(db),
    m_mode(RailwayNodeMode::StationLinesMode)
{

}

ISqlFKMatchModel *StationOrLineMatchFactory::createModel()
{
    switch (m_mode)
    {
    case RailwayNodeMode::StationLinesMode:
    {
        return new LinesMatchModel(mDb, false);
    }
    case RailwayNodeMode::LineStationsMode:
    {
        StationsMatchModel *m = new StationsMatchModel(mDb);
        m->setFilter(0, 0);
        return m;
    }
    }
    return nullptr;
}
