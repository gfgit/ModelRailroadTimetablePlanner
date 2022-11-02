#include "jobcrossingtask.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

inline bool fillCrossingErrorData(query::rows& job, JobCrossingErrorData& err, bool first, JobCategory &outCat)
{
    err.stopId = job.get<db_id>(0);
    err.otherJob.stopId = job.get<db_id>(1);
    err.jobId = job.get<db_id>(2);
    outCat = JobCategory(job.get<int>(3));
    err.otherJob.jobId = job.get<db_id>(4);
    err.otherJob.category = JobCategory(job.get<int>(5));
    err.departure = job.get<QTime>(6);
    err.arrival = job.get<QTime>(7);
    err.otherDep = job.get<QTime>(8);
    err.otherArr = job.get<QTime>(9);
    bool passing = job.get<int>(10) == 1;
    err.stationId = job.get<db_id>(11);
    err.stationName = job.get<QString>(12);

    if(passing)
    {
        //In passings:
        //job A starts before B but gets passed and ends after B
        //job B starts after A but ends before
        //B travel period is contained in A travel period

        //We need stricter checking of time, one travel must be contained in the other
        if(err.departure < err.otherDep && err.arrival < err.otherArr)
            return false; //A travels before B, no passing
        if(err.departure > err.otherDep && err.arrival > err.otherArr)
            return false; //A travels after B, no passing
    }

    err.type = passing ? JobCrossingErrorData::JobPassing : JobCrossingErrorData::JobCrossing;

    if(!first)
    {
        //Swap points of view
        qSwap(err.jobId, err.otherJob.jobId);
        qSwap(outCat, err.otherJob.category);
        qSwap(err.stopId, err.otherJob.stopId);
        qSwap(err.arrival, err.otherArr);
        qSwap(err.departure, err.otherDep);
    }

    return true;
}

JobCrossingResultEvent::JobCrossingResultEvent(JobCrossingTask *worker, const JobCrossingErrorMap::ErrorMap &data, bool merge) :
    GenericTaskEvent(_Type, worker),
    results(data),
    mergeErrors(merge)
{

}

JobCrossingTask::JobCrossingTask(sqlite3pp::database &db, QObject *receiver, const QVector<db_id>& jobs) :
    IQuittableTask(receiver),
    mDb(db),
    jobsToCheck(jobs)
{

}

void JobCrossingTask::run()
{
    //TODO: allow checking a single job

    //Look for passing or crossings on same segment
    query q(mDb, "SELECT s1.id, s2.id, s1.job_id, j1.category, s2.job_id, j2.category,"
                 "s1.departure, MIN(s1_next.arrival),"
                 "s2.departure, MIN(s2_next.arrival),"
                 "g1.gate_id=g2.gate_id," //1 = passing, 0 = crossing (opposite direction)
                 "s1.station_id, stations.name"
                 " FROM stops s1"
                 " JOIN stops s1_next ON s1_next.job_id=s1.job_id AND s1_next.arrival>s1.arrival"
                 " JOIN stops s2 ON s2.next_segment_conn_id=s1.next_segment_conn_id AND s2.id<>s1.id"
                 " JOIN stops s2_next ON s2_next.job_id=s2.job_id AND s2_next.arrival>s2.arrival"
                 " JOIN jobs j1 ON j1.id=s1.job_id"
                 " JOIN jobs j2 ON j2.id=s2.job_id"
                 " JOIN station_gate_connections g1 ON g1.id=s1.out_gate_conn"
                 " JOIN station_gate_connections g2 ON g2.id=s2.out_gate_conn"
                 " JOIN stations ON stations.id=s1.station_id"
                 " GROUP BY s1.id,s2.id"
                 " HAVING s1.departure<=s2_next.arrival AND s1_next.arrival>=s2.departure");

    QMap<db_id, JobCrossingErrorList> errorMap;
    checkCrossAndPassSegments(errorMap, q);

    sendEvent(new JobCrossingResultEvent(this, errorMap, false), true);

    //    if(jobsToCheck.isEmpty())
    //    {
    //        //Select all jobs
    //        query q_getAll(mDb, "SELECT id,category FROM jobs");
    //        for(auto job : q_getAll)
    //        {
    //            JobErrorList list;
    //            list.jobId = job.get<db_id>(0);
    //            list.jobCategory = JobCategory(job.get<int>(1));

    //            checkCrossAndPassSegments(list, &q);
    //        }
    //    }
    //    else
    //    {
    //        query q_getCat(mDb, "SELECT category FROM jobs WHERE id=?");
    //        for(const db_id jobId : qAsConst(jobsToCheck))
    //        {
    //            JobErrorList list;
    //            list.jobId = jobId;
    //            list.jobCategory = JobCategory::NCategories;

    //            q_getCat.bind(1, jobId);
    //            if(q_getCat.step() == SQLITE_ROW)
    //            {
    //                list.jobCategory = JobCategory(q_getCat.getRows().get<int>(0));
    //            }
    //            q_getCat.reset();

    //            if(list.jobCategory == JobCategory::NCategories)
    //                continue; //Job doesn't exist, skip TODO: remove from error widget

    //            checkCrossAndPassSegments(list, &q);
    //        }
    //    }
}

void JobCrossingTask::checkCrossAndPassSegments(JobCrossingErrorMap::ErrorMap &errMap, sqlite3pp::query &q)
{
    for(auto job : q)
    {
        JobCrossingErrorData err;
        JobCategory category = JobCategory::NCategories;

        if(!fillCrossingErrorData(job, err, true, category))
            continue;

        auto it = errMap.find(err.jobId);
        if(it == errMap.end())
        {
            //Insert Job into map for first time
            JobCrossingErrorList list;
            list.job.jobId = err.jobId;
            list.job.category = category;

            it = errMap.insert(list.job.jobId, list);
        }

        it.value().errors.append(err);
    }

    q.reset();
}
