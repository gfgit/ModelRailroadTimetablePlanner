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

#include "jobcrossingmodel.h"

#include "utils/jobcategorystrings.h"

static const char *error_texts[] = {
  nullptr, QT_TRANSLATE_NOOP("JobErrors", "Job crosses another Job on same track."),
  QT_TRANSLATE_NOOP("JobErrors", "Job passes another Job on same track.")};

class JobErrors
{
    Q_DECLARE_TR_FUNCTIONS(JobErrors)
};

JobCrossingModel::JobCrossingModel(QObject *parent) :
    JobCrossingModelBase(parent)
{
}

QVariant JobCrossingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
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
    if (!idx.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const JobCrossingErrorData *item = getItem(idx);
    if (item)
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
            return JobErrors::tr(error_texts[item->type]);
        default:
            break;
        }
    }
    else
    {
        // Caption
        if (idx.row() >= m_data.topLevelCount() || idx.column() != 0)
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

void JobCrossingModel::removeJob(db_id jobId)
{
    beginResetModel();
    m_data.removeJob(jobId);
    endResetModel();
}

void JobCrossingModel::renameJob(db_id newJobId, db_id oldJobId)
{
    beginResetModel();
    m_data.renameJob(newJobId, oldJobId);
    endResetModel();
}

void JobCrossingModel::renameStation(db_id stationId, const QString &name)
{
}
