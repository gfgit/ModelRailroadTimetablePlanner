#ifndef LINEGRAPHSCENE_H
#define LINEGRAPHSCENE_H

#include <QObject>
#include <QVector>
#include <QHash>

#include <QPointF>
#include <QSize>

#include "utils/types.h"

#include "graph/linegraphtypes.h"

#include "stationgraphobject.h"

namespace sqlite3pp {
class database;
}

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

    /*!
     * \brief Load graph contents
     *
     * Loads stations and jobs
     *
     * \param objectId Graph object ID
     * \param type Graph type
     * \param force Force reloading if objectId and type are the same as current
     *
     * \sa loadStation()
     * \sa loadFullLine()
     * \sa reloadJobs()
     */
    bool loadGraph(db_id objectId, LineGraphType type, bool force = false);

    /*!
     * \brief Load graph jobs
     *
     * Reloads only jobs but not stations
     * \sa loadGraph()
     */
    bool reloadJobs();

    /*!
     * \brief Get job at graph position
     *
     * \param pos Point in scene coordinates
     * \param tolerance A tolerance if mouse doesn't exactly click on job item
     */
    JobStopEntry getJobAt(const QPointF& pos, const double tolerance);

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

    JobStopEntry getSelectedJob() const;
    void setSelectedJobId(JobStopEntry stop);

signals:
    void graphChanged(int type, db_id objectId);
    void redrawGraph();
    void jobSelected(db_id jobId);

public slots:
    /*!
     * \brief Reload everything
     *
     * \param pos Point in scene coordinates
     * \param tolerance A tolerance if mouse doesn't exactly click on job item
     */
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
    /*!
     * \brief Recalculate and store content size
     *
     * Stores cached calculated size of the graph.
     * It depends on settings like station offset
     * \sa MRTPSettings
     */
    void recalcContentSize();

    /*!
     * \brief Load station details
     *
     * Loads station name and tracks (color, attributes, names)
     * It does NOT load jobs, only tracks
     */
    bool loadStation(StationGraphObject &st);

    /*!
     * \brief Load all stations in a railway line
     *
     * Loads stations of all railway line segments
     * \sa loadStation()
     */
    bool loadFullLine(db_id lineId);

    /*!
     * \brief Load job stops in station
     *
     * Load jobs stops, only the part on station's track
     * \sa loadSegmentJobs()
     */
    bool loadStationJobStops(StationGraphObject &st);

    /*!
     * \brief Load job segments between 2 stations
     *
     * Load job segments and stores them in 'from' station
     * \sa loadStationJobStops()
     */
    bool loadSegmentJobs(StationPosEntry &stPos, const StationGraphObject &fromSt, const StationGraphObject &toSt);

private:
    friend class BackgroundHelper;
    friend class LineGraphManager;

    sqlite3pp::database& mDb;

    /*!
     * \brief Graph Object ID
     *
     * Can be station, segment, line ID
     * Depending on graph type
     */
    db_id graphObjectId;

    /*!
     * \brief Type of Graph displayed by Scene
     */
    LineGraphType graphType;

    /*!
     * \brief Graph Object Name
     *
     * Can be station, segment, line name
     * Depending on graph type
     */
    QString graphObjectName;

    QVector<StationPosEntry> stationPositions;
    QHash<db_id, StationGraphObject> stations;

    /*!
     * \brief Graph size
     *
     * Caches calculated size of the graph.
     * So it doesn't need to be recalculated for scrollbars and printing
     */
    QSize contentSize;

    /*!
     * \brief Job selection
     *
     * Caches selected job item ID
     */
    JobStopEntry selectedJob;
};

#endif // LINEGRAPHSCENE_H
