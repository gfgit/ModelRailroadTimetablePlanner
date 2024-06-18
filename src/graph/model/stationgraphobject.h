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

#ifndef STATIONGRAPHOBJECT_H
#define STATIONGRAPHOBJECT_H

#include <QList>

#include "utils/types.h"

#include "stations/station_utils.h"
#include <QRgb>

/*!
 * \brief Graph of a railway station
 *
 * Contains informations to draw station name, platforms and jobs
 *
 * \sa PlatformGraph
 */
class StationGraphObject
{
public:
    StationGraphObject();

    db_id stationId;
    QString stationName;
    utils::StationType stationType;

    /*!
     * \brief Graph of the job while is stopping
     *
     * Contains informations to draw job line on top of the PlatformGraph
     */
    struct JobStopGraph
    {
        JobStopEntry stop;
        double arrivalY;
        double departureY;
        bool drawLabel;
    };

    /*!
     * \brief Graph of a station track (platform)
     *
     * Contains informations to draw platform line and header name
     * \sa JobStopGraph
     */
    struct PlatformGraph
    {
        db_id platformId;
        QString platformName;
        QRgb color;
        QFlags<utils::StationTrackType> platformType;
        QList<JobStopGraph> jobStops;
    };

    QList<PlatformGraph> platforms;

    double xPos;
};

#endif // STATIONGRAPHOBJECT_H
