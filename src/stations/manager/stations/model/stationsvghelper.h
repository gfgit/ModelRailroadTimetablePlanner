#ifndef STATIONSVGHELPER_H
#define STATIONSVGHELPER_H

#include "utils/types.h"
#include "stations/station_utils.h"

#include <QCoreApplication> //For transalations

#include <QTime>

class QIODevice;

namespace sqlite3pp {
class database;
}

namespace ssplib {
class StationPlan;
}

struct StationSVGJobStops
{
    struct Stop
    {
        struct Gate
        {
            db_id gateId = 0;
            db_id gateTrackNum = 0;
            db_id trackId = 0;
            utils::Side trackSide = 0;
        };

        JobStopEntry job;
        Gate in_gate;
        Gate out_gate;
        QTime arrival;
        QTime departure;
    };

    db_id stationId = 0;
    QTime time;
    QVector<Stop> stops;
};

//FIXME: compress SVG on saving and uncompress on loading?
class StationSVGHelper
{
    Q_DECLARE_TR_FUNCTIONS(Odt)
public:
    static bool stationHasSVG(sqlite3pp::database &db, db_id stationId, QString *stNameOut);

    static bool addImage(sqlite3pp::database &db, db_id stationId, QIODevice *source,
                         QString *errOut = nullptr);
    static bool removeImage(sqlite3pp::database &db, db_id stationId,
                            QString *errOut = nullptr);

    static bool saveImage(sqlite3pp::database &db, db_id stationId, QIODevice *dest,
                          QString *errOut = nullptr);
    static QIODevice* loadImage(sqlite3pp::database &db, db_id stationId);

    static bool loadStationFromDB(sqlite3pp::database &db, db_id stationId,
                                  QString &stName, ssplib::StationPlan *plan);

    static bool loadStationJobsFromDB(sqlite3pp::database &db, StationSVGJobStops *station);

    static bool applyStationJobsToPlan(const StationSVGJobStops *station, ssplib::StationPlan *plan);
};

#endif // STATIONSVGHELPER_H
