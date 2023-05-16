/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
