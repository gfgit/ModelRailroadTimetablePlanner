#ifndef JOBCROSSINGTASK_H
#define JOBCROSSINGTASK_H

#include <QVector>
#include <QEvent>

#include "utils/thread/iquittabletask.h"
#include "utils/thread/taskprogressevent.h"

#include "job_crossing_data.h"

namespace sqlite3pp {
class database;
class query;
}

class JobCrossingTask;

class JobCrossingResultEvent : public GenericTaskEvent
{
public:
    static const Type _Type = Type(CustomEvents::JobsCrossingCheckResult);

    JobCrossingResultEvent(JobCrossingTask *worker, const JobCrossingErrorMap::ErrorMap &data, bool merge);

    QMap<db_id, JobCrossingErrorList> results;
    bool mergeErrors;
};

class JobCrossingTask : public IQuittableTask
{
public:
    JobCrossingTask(sqlite3pp::database &db, QObject *receiver, const QVector<db_id>& jobs);

    void run() override;

    void checkCrossAndPassSegments(JobCrossingErrorMap::ErrorMap& errMap, sqlite3pp::query &q);

private:
    sqlite3pp::database &mDb;

    QVector<db_id> jobsToCheck;
};

#endif // JOBCROSSINGTASK_H
