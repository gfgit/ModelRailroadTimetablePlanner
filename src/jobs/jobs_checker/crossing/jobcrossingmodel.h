#ifndef JOBCROSSINGMODEL_H
#define JOBCROSSINGMODEL_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include "utils/singledepthtreemodelhelper.h"

#include "job_crossing_data.h"

class JobCrossingModel;
typedef SingleDepthTreeModelHelper<JobCrossingModel, JobCrossingErrorMap, JobCrossingErrorData> JobCrossingModelBase;

//TODO: make on-demand
class JobCrossingModel : public JobCrossingModelBase
{
    Q_OBJECT

public:
    enum Columns
    {
        JobName = 0,
        StationName,
        Arrival,
        Departure,
        Description,
        NCols
    };

    JobCrossingModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void setErrors(const QMap<db_id, JobCrossingErrorList> &data);

    void mergeErrors(const JobCrossingErrorMap::ErrorMap &errMap);

    void clear();
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // JOBCROSSINGMODEL_H
