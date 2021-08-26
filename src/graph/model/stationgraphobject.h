#ifndef STATIONGRAPHOBJECT_H
#define STATIONGRAPHOBJECT_H

#include <QVector>

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
    typedef struct
    {
        db_id jobId;
        JobCategory category;

        db_id stopId;
        double arrivalY;
        double departureY;
    } JobStopGraph;

    /*!
     * \brief Graph of a station track (platform)
     *
     * Contains informations to draw platform line and header name
     * \sa JobStopGraph
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
