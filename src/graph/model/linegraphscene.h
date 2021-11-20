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
     * It also updates current job selection
     * \sa loadGraph()
     * \sa updateJobSelection()
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
    void setSelectedJob(JobStopEntry stop);

    /*!
     * \brief activate scene
     * \param self a pointer to LineGraphScene instance
     *
     * For scenes registered on a LineGraphManager, this tells
     * our instance is now the active one and will therefore receive
     * user requests to show items
     *
     * \sa LineGraphManager::setActiveScene()
     * \sa sceneActivated()
     */
    inline void activateScene() { emit sceneActivated(this); }

    /*!
     * \brief requestShowZone
     * \param stationId null if you want to select segment
     * \param segmentId null if you want to select station
     * \param from top of the zone
     * \param to bottom of the zone
     *
     * Calculates a zone in the scene which show the selected station or entire segment
     * (It depends on which is non-null)
     * The y values are calculated from the QTime arguments
     * Then it requests the view to show the area.
     *
     * \sa requestShowRect()
     */
    bool requestShowZone(db_id stationId, db_id segmentId, QTime from, QTime to);

signals:
    void graphChanged(int type, db_id objectId);
    void redrawGraph();

    /*!
     * \brief job selected
     * \param jobId ID of the job on 0 if selection cleared.
     * \param category job's category casted to int
     * \param stopId possible stop ID hint to select a specific section
     *
     * Selection changed: either user clicked on a job
     * or on an empty zone to clear selection
     */
    void jobSelected(db_id jobId, int category, db_id stopId);

    /*!
     * \brief Signal for activation
     * \param self a pointer to LineGraphScene instance
     *
     * \sa activateScene()
     */
    void sceneActivated(LineGraphScene *self);

    /*!
     * \brief request LineGraphView to show this rect
     *
     * The view will ensure this rect is visible
     */
    void requestShowRect(const QRectF& rect);

public slots:
    /*!
     * \brief Reload everything
     *
     * Reload entire contents
     * \sa loadSegmentJobs()
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
     * \brief Get job stop at graph position
     *
     * \param prevSt Station at position's left
     * \param nextSt Station at position's right
     * \param pos Point in scene coordinates
     * \param tolerance A tolerance if mouse doesn't exactly click on job item
     *
     * Check if a job stop in this station matches requested position
     * Otherwise return null selection
     */
    JobStopEntry getJobStopAt(const StationGraphObject *prevSt, const StationGraphObject *nextSt,
                              const QPointF &pos, const double tolerance);

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

    /*!
     * \brief Update job selection category
     *
     * Updates current selection.
     * If selected job got removed or changed ID (Number) selection is cleared
     * If selected stop got removed or doesn't belong to selected job it's cleared
     * Category of selected job gets updated if changed in the meantime
     */
    void updateJobSelection();

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
