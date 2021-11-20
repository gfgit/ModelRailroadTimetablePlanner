#ifndef LINEGRAPHSELECTIONHELPER_H
#define LINEGRAPHSELECTIONHELPER_H

#include "utils/types.h"
#include "graph/linegraphtypes.h"
#include <QTime>

class LineGraphScene;

namespace sqlite3pp {
class database;
}

class LineGraphSelectionHelper
{
public:

    /*!
     * \brief The SegmentInfo struct
     */
    struct SegmentInfo
    {
        db_id segmentId = 0;
        db_id firstStId = 0;
        db_id secondStId = 0;
        db_id firstStopId = 0;

        /*!
         * \brief arrival and start
         *
         * Input: hour time limit after which stops are searched.
         * Output: first stop arrival
         */
        QTime arrivalAndStart;
        QTime departure; //!< departure of first or second stop (if found)
    };

    LineGraphSelectionHelper(sqlite3pp::database &db);

    //Low level API

    /*!
     * \brief find job in current graph
     * \param scene the graph scene
     * \param jobId job ID
     * \param info structure to get output result
     * \return true if found
     *
     * Try to find a job stop in current graph.
     * If scene has NoGraph or does not contain requested job then returns false
     * SegmentInfo::secondStId is always left empty
     */
    bool tryFindJobStopInGraph(LineGraphScene *scene, db_id jobId, SegmentInfo& info);

    /*!
     * \brief find 2 job stops after requested hour
     * \param jobId job ID
     * \param info structure to get output result, and pass SegmentInfo::arrivalAndStart
     * \return true if found 1 or 2 stops
     *
     * Try to find 2 job stops after requested hour (SegmentInfo::arrivalAndStart).
     * If only 1 stop is found, SegmentInfo::secondStId is 0.
     */
    bool tryFindJobStopsAfter(db_id jobId, SegmentInfo &info);

    /*!
     * \brief try find a new graph for job
     * \param jobId job ID
     * \param info structure to get output result, and pass SegmentInfo::arrivalAndStart
     * \param outGraphObjId output object to load in scene
     * \param outGraphType graph scene type
     * \return false if not found
     *
     * Try to find a railway line containing the job. If not found try with a railway segment.
     * If neither line nor segment are found try with first stop station.
     */
    bool tryFindNewGraphForJob(db_id jobId, SegmentInfo &info, db_id &outGraphObjId, LineGraphType &outGraphType);

public:
    //High level API

    /*!
     * \brief request job selection
     * \param scene the graph scene
     * \param jobId jobId
     * \param select do you want to select the job?
     * \param ensureVisible do you want to make it visible on LineGraphView
     * \return true on success
     *
     * Try to select or make visible (or both) the requested job on current scene graph.
     * If the job is not found on current graph, it tries to find a new grah and loads it.
     *
     * \sa ViewManager::requestJobSelection()
     */
    bool requestJobSelection(LineGraphScene *scene, db_id jobId, bool select, bool ensureVisible);

private:
    sqlite3pp::database &mDb;
};

#endif // LINEGRAPHSELECTIONHELPER_H
