/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
            db_id connId = 0;
            db_id gateId = 0;
            db_id gateTrackNum = 0;
            db_id trackId = 0;
            utils::Side trackSide = utils::Side::NSides;
        };

        JobStopEntry job;
        Gate in_gate;
        Gate out_gate;
        QTime arrival;
        QTime departure;
        db_id next_seg_conn = 0;
        StopType type;
    };

    db_id stationId = 0;
    QVector<Stop> stops;
    QTime time;
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
                                  ssplib::StationPlan *plan, bool onlyAlreadyPresent);

    static bool getPrevNextStop(sqlite3pp::database &db, db_id stationId,
                                bool next, QTime &time);

    static bool loadStationJobsFromDB(sqlite3pp::database &db, StationSVGJobStops *station);

    static bool applyStationJobsToPlan(const StationSVGJobStops *station, ssplib::StationPlan *plan);

    static bool writeStationXml(QIODevice *dev, ssplib::StationPlan *plan);

    static bool writeStationXmlFromDB(sqlite3pp::database &db, db_id stationId, QIODevice *dev);

    static bool importTrackConnFromSVG(sqlite3pp::database &db, db_id stationId, ssplib::StationPlan *plan);
    static bool importTrackConnFromSVGDev(sqlite3pp::database &db, db_id stationId, QIODevice *dev);
};

#endif // STATIONSVGHELPER_H
