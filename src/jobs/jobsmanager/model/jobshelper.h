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
    static bool createNewJob(sqlite3pp::database &db, db_id &outJobId);
    static bool removeJob(sqlite3pp::database &db, db_id jobId);
    static bool removeAllJobs(sqlite3pp::database &db);

    static bool copyStops(sqlite3pp::database &db, db_id fromJobId, db_id toJobId,
                          int secsOffset, bool copyRsOps);
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
