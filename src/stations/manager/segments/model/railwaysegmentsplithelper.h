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

#ifndef RAILWAYSEGMENTSPLITHELPER_H
#define RAILWAYSEGMENTSPLITHELPER_H

#include "utils/types.h"
#include "stations/station_utils.h"

#include <QString>

namespace sqlite3pp {
class database;
} // namespace sqlite3pp

class RailwaySegmentConnectionsModel;

class RailwaySegmentSplitHelper
{
public:
    RailwaySegmentSplitHelper(sqlite3pp::database &db, RailwaySegmentConnectionsModel *origSegConn,
                              RailwaySegmentConnectionsModel *newSegConn);

    void setInfo(const utils::RailwaySegmentInfo &origInfo,
                 const utils::RailwaySegmentInfo &newInfo);

    bool split();

private:
    bool updateLines();

private:
    sqlite3pp::database &mDb;

    RailwaySegmentConnectionsModel *origSegConnModel;
    RailwaySegmentConnectionsModel *newSegConnModel;

    utils::RailwaySegmentInfo origSegInfo;
    utils::RailwaySegmentInfo newSegInfo;
};

#endif // RAILWAYSEGMENTSPLITHELPER_H
