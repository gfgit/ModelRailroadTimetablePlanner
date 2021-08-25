#ifndef STATIONGRAPHOBJECT_H
#define STATIONGRAPHOBJECT_H

#include <QVector>

#include "utils/types.h"

#include "stations/station_utils.h"
#include <QRgb>

/*!
 * Graph of a railway station
 *
 * Contains informations to draw station name, platforms and jobs
 */
class StationGraphObject
{
public:
    StationGraphObject();

    db_id stationId;
    QString stationName;
    utils::StationType stationType;

    /*!
     * Graph of the job while is stopping
     *
     * Contains informations to draw job line on top of the PlatformGraph
     */
    typedef struct
    {
        db_id jobId;
        JobCategory category;

        db_id stopId;
        double arrivalY;
        double departureY;
    } JobStopGraph;

    /*!
     * Graph of the job while is moving
     *
     * Contains informations to draw job line between two adjacent stations
     */
    typedef struct
    {
        db_id jobId;
        JobCategory category;

        db_id fromStopId;
        db_id fromPlatfId;
        double fromDepartureY;

        db_id toStopId;
        db_id toPlatfId;
        double toArrivalY;
    } JobLineGraph;

    /*!
     * Graph of a station track (platform)
     *
     * Contains informations to draw platform line and header name
     */
    typedef struct
    {
        db_id platformId;
        QString platformName;
        QRgb color;
        QFlags<utils::StationTrackType> platformType;
        QVector<JobStopGraph> jobStops;
    } PlatformGraph;

    QVector<PlatformGraph> platforms;

    double xPos;
};

#endif // STATIONGRAPHOBJECT_H
