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

#ifndef STATIONWRITER_H
#define STATIONWRITER_H

#include <QList>
#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class QXmlStreamWriter;

class StationWriter
{
public:
    StationWriter(database &db);

    static void writeStationAutomaticStyles(QXmlStreamWriter &xml);

    void writeStation(QXmlStreamWriter &xml, db_id stationId, QString *stNameOut = nullptr);

private:
    struct Stop
    {
        db_id jobId;

        QString prevSt;
        QString nextSt;

        QString description;
        QString platform;

        QTime arrival;
        QTime departure;

        JobCategory jobCat;
    };

    void insertStop(QXmlStreamWriter &xml, const Stop &stop, bool first, bool transit);

private:
    database &mDb;

    query q_getJobsByStation;
    query q_selectPassings;
    query q_getStopCouplings;
};

#endif // STATIONWRITER_H
