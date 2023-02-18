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

    void removeJob(db_id jobId);

    void renameJob(db_id newJobId, db_id oldJobId);

    void renameStation(db_id stationId, const QString& name);
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // JOBCROSSINGMODEL_H
