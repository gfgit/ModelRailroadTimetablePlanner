#include "jobcrossingmodel.h"

#include "utils/jobcategorystrings.h"

JobCrossingModel::JobCrossingModel(QObject *parent) : JobCrossingModelBase(parent)
{

}

QVariant JobCrossingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case JobName:
            return tr("Job");
        case StationName:
            return tr("Station");
        case Arrival:
            return tr("Arrival");
        case Departure:
            return tr("Departure");
        case Description:
            return tr("Description");
        default:
            break;
        }
    }

    return JobCrossingModelBase::headerData(section, orientation, role);
}

QVariant JobCrossingModel::data(const QModelIndex &idx, int role) const
{
    if(!idx.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const JobCrossingErrorData *item = getItem(idx);
    if(item)
    {
        switch (idx.column())
        {
        case JobName:
            return JobCategoryName::jobName(item->otherJob.jobId, item->otherJob.category);
        case StationName:
            return item->stationName;
        case Arrival:
            return item->arrival;
        case Departure:
            return item->departure;
        case Description:
            break; //TODO
        default:
            break;
        }
    }
    else
    {
        //Caption
        if(idx.row() >= m_data.topLevelCount() || idx.column() != 0)
            return QVariant();

        auto topLevel = m_data.getTopLevelAtRow(idx.row());
        return JobCategoryName::jobName(topLevel->job.jobId, topLevel->job.category);
    }

    return QVariant();
}

void JobCrossingModel::setErrors(const JobCrossingErrorMap::ErrorMap &errMap)
{
    beginResetModel();
    m_data.map = errMap;
    endResetModel();
}

void JobCrossingModel::mergeErrors(const JobCrossingErrorMap::ErrorMap &errMap)
{
    beginResetModel();
    m_data.merge(errMap);
    endResetModel();
}

void JobCrossingModel::clear()
{
    beginResetModel();
    m_data.map.clear();
    endResetModel();
}
