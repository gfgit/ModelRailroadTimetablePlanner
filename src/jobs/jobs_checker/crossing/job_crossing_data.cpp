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

#include "job_crossing_data.h"

JobCrossingErrorMap::JobCrossingErrorMap()
{
}

void JobCrossingErrorMap::removeJob(db_id jobId)
{
    auto job = map.find(jobId);
    if (job == map.end())
        return; // Not contained in map

    // Remove all errors referencing to us
    for (const JobCrossingErrorData &err : qAsConst(job->errors))
    {
        auto otherJob = map.find(err.otherJob.jobId);
        if (otherJob == map.end())
            continue; // Maybe already remove, skip

        // Remove all errors regarding job in otherJob
        std::remove_if(otherJob->errors.begin(), otherJob->errors.end(),
                       [jobId](const JobCrossingErrorData &otherErr) -> bool
                       { return otherErr.otherJob.jobId == jobId; });

        if (otherJob->errors.isEmpty())
        {
            // otherJob has no errors, remove it
            map.erase(otherJob);
        }
    }

    // Remove job
    map.erase(job);
}

void JobCrossingErrorMap::renameJob(db_id newJobId, db_id oldJobId)
{
    auto job = map.find(oldJobId);
    if (job == map.end())
        return; // Not contained in map

    // Uodate all errors referencing to us
    for (const JobCrossingErrorData &err : qAsConst(job->errors))
    {
        auto otherJob = map.find(err.otherJob.jobId);
        if (otherJob == map.end())
            continue; // Maybe already remove, skip

        for (JobCrossingErrorData &otherErr : otherJob->errors)
        {
            if (otherErr.otherJob.jobId == oldJobId)
            {
                otherErr.otherJob.jobId = newJobId;
            }
        }
    }

    // Update job
    auto errList      = map.take(oldJobId);
    errList.job.jobId = newJobId;
    map.insert(newJobId, errList);
}

void JobCrossingErrorMap::merge(const ErrorMap &results)
{
    for (const JobCrossingErrorList &list : results)
    {
        // First clear Job
        removeJob(list.job.jobId);

        // Then add new errors if not empty (already duplicated)
        if (!list.errors.isEmpty())
            map.insert(list.job.jobId, list);
    }
}
