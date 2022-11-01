#ifndef JOB_CROSS_DATA_H
#define JOB_CROSS_DATA_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include <QMap>
#include <QTime>

#include "utils/types.h"

struct JobCrossingErrorData
{
    db_id jobId;

    db_id stopId;
    db_id stationId;

    JobStopEntry otherJob;

    QTime arrival, departure;
    QTime otherArr, otherDep;

    QString stationName;

    enum Type
    {
        NoError = 0,
        JobCrossing, //NOTE: arrival refers to next job stop so it comes after departure (same for otherArr/Dep)
        JobPassing
    };

    Type type;
};

struct JobCrossingErrorList
{
    JobEntry job;
    QVector<JobCrossingErrorData> errors;

    inline int childCount() const { return errors.size(); }
    inline const JobCrossingErrorData *ptrForRow(int row) const { return &errors.at(row); }
};

/*!
 * \brief The JobCrossingErrorMap class
 *
 * Each job crossing or passing involves 2 jobs.
 * This map contains duplicated errors from the point of view
 * of both jobs.
 */
class JobCrossingErrorMap
{
public:
    typedef QMap<db_id, JobCrossingErrorList> ErrorMap;

    JobCrossingErrorMap();

    inline int topLevelCount() const { return map.size(); }

    inline const JobCrossingErrorList *getTopLevelAtRow(int row) const
    {
        if(row >= topLevelCount())
            return nullptr;
        return &(map.constBegin() + row).value();
    }

    inline const JobCrossingErrorList *getParent(JobCrossingErrorData *child) const
    {
        auto it = map.constFind(child->jobId);
        if(it == map.constEnd())
            return nullptr;
        return &it.value();
    }

    inline int getParentRow(JobCrossingErrorData *child) const
    {
        auto it = map.constFind(child->jobId);
        if(it == map.constEnd())
            return -1;
        return std::distance(map.constBegin(), it);
    }

    void removeJob(db_id jobId);

    void merge(const ErrorMap& results);

public:
    ErrorMap map;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // JOB_CROSS_DATA_H
