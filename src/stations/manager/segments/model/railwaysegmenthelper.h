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

#ifndef RAILWAYSEGMENTHELPER_H
#define RAILWAYSEGMENTHELPER_H

#include "utils/types.h"
#include "stations/station_utils.h"

#include <QString>

namespace sqlite3pp {
class database;
} // namespace sqlite3pp

class RailwaySegmentHelper
{
public:
    RailwaySegmentHelper(sqlite3pp::database &db);

    bool getSegmentInfo(utils::RailwaySegmentInfo& info);

    bool getSegmentInfoFromGate(db_id gateId, utils::RailwaySegmentInfo& info);

    bool setSegmentInfo(db_id& segmentId, bool create,
                        const QString &name, utils::RailwaySegmentType type,
                        int distance, int speed,
                        db_id fromGateId, db_id toGateId,
                        QString *outErrMsg);

    bool removeSegment(db_id segmentId, QString *outErrMsg);

    bool findFirstLineOrSegment(db_id &graphObjId, bool &isLine);

private:
    sqlite3pp::database &mDb;
};

#endif // RAILWAYSEGMENTHELPER_H
