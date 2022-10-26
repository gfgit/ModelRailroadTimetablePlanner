#ifndef JOBSHELPER_H
#define JOBSHELPER_H

#include "utils/types.h"
#include "stations/station_utils.h"

namespace sqlite3pp {
class database;
class query;
}

class JobsHelper
{
public:
    /*!
     * \brief createNewJob
     * \param db open database connection
     * \param outJobId ID of created job
     * \param cat Job category
     * \return true on success
     *
     * If \a outJobId is passed non zero, the function tries to cread a Job with the passed ID
     * If the Job ID is already in use, the function fails and no new Job is created.
     * If 0 is passed, the function creates a new Job with arbitrary free ID from database
     */
    static bool createNewJob(sqlite3pp::database &db, db_id &outJobId, JobCategory cat = JobCategory::FREIGHT);
    static bool removeJob(sqlite3pp::database &db, db_id jobId);
    static bool removeAllJobs(sqlite3pp::database &db);

    static bool copyStops(sqlite3pp::database &db, db_id fromJobId, db_id toJobId,
                          int secsOffset, bool copyRsOps, bool reversePath);

    static bool checkShiftsExist(sqlite3pp::database &db);
};

class JobStopDirectionHelper
{
public:
    JobStopDirectionHelper(sqlite3pp::database &db);
    ~JobStopDirectionHelper();

    utils::Side getStopOutSide(db_id stopId);

private:
    sqlite3pp::database &mDb;
    sqlite3pp::query *m_query;
};

#endif // JOBSHELPER_H
