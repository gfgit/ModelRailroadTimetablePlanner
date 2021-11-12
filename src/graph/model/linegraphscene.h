#ifndef LINEGRAPHSCENE_H
#define LINEGRAPHSCENE_H

#include <QObject>
#include <QVector>
#include <QHash>

#include <QPointF>
#include <QSize>

#include "utils/types.h"

#include "stationgraphobject.h"

namespace sqlite3pp {
class database;
}

/*!
 * \brief Enum to describe view content type
 */
enum class LineGraphType
{
    NoGraph = 0, //!< No content displayed
    SingleStation, //!< Show a single station
    RailwaySegment, //!< Show two adjacent stations and the segment in between
    RailwayLine //!< Show a complete railway line (multiple adjacent segments)
};

/*!
 * \brief Class to store line information
 *
 * Stores information to draw railway content in a LineGraphView
 *
 * \sa LineGraphManager
 * \sa LineGraphView
 */
class LineGraphScene : public QObject
{
    Q_OBJECT
public:
    LineGraphScene(sqlite3pp::database &db, QObject *parent = nullptr);

    bool loadGraph(db_id objectId, LineGraphType type, bool force = false);

    bool reloadJobs();

    JobEntry getJobAt(const QPointF& pos, const double tolerance);

    inline LineGraphType getGraphType() const
    {
        return graphType;
    }

    inline db_id getGraphObjectId() const
    {
        return graphObjectId;
    }

    inline QString getGraphObjectName() const
    {
        return graphObjectName;
    }

    inline QSize getContentSize() const
    {
        return contentSize;
    }

signals:
    void graphChanged(int type, db_id objectId);
    void redrawGraph();

public slots:
    void reload();

private:

    /*!
     * \brief Graph of the job while is moving
     *
     * Contains informations to draw job line between two adjacent stations
     */
    typedef struct
    {
        db_id jobId;
        JobCategory category;

        db_id fromStopId;
        db_id fromPlatfId;
        QPointF fromDeparture;

        db_id toStopId;
        db_id toPlatfId;
        QPointF toArrival;
    } JobSegmentGraph;

    /*!
     * \brief Station entry on scene
     *
     * Represents a station item placeholder in an ordered list of scene
     */
    typedef struct
    {
        db_id stationId;
        db_id segmentId;
        double xPos;

        QVector<JobSegmentGraph> nextSegmentJobGraphs;
        /*!<
         * Stores job graph of the next segment
         * Which means jobs departing from this staation and going to next one
         */
    } StationPosEntry;

private:
    void recalcContentSize();
    bool loadStation(StationGraphObject &st);
    bool loadFullLine(db_id lineId);

    bool loadStationJobStops(StationGraphObject &st);
    bool loadSegmentJobs(StationPosEntry &stPos, const StationGraphObject &fromSt, const StationGraphObject &toSt);

private:
    friend class BackgroundHelper;
    friend class LineGraphManager;

    sqlite3pp::database& mDb;

    /* Can be station/segment/line ID depending on graph type */
    db_id graphObjectId;
    LineGraphType graphType;

    QString graphObjectName;

    QVector<StationPosEntry> stationPositions;
    QHash<db_id, StationGraphObject> stations;

    QSize contentSize;
};

#endif // LINEGRAPHSCENE_H
