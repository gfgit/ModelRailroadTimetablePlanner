#ifndef JOBSHELPER_H
#define JOBSHELPER_H

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class JobsHelper
{
public:
    static bool createNewJob(sqlite3pp::database &db, db_id &outJobId);
    static bool removeJob(sqlite3pp::database &db, db_id jobId);
    static bool removeAllJobs(sqlite3pp::database &db);
};

#endif // JOBSHELPER_H
