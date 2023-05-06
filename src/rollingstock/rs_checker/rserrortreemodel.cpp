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

#ifdef ENABLE_BACKGROUND_MANAGER

#include "rserrortreemodel.h"

#include <QCoreApplication> //For translations

#include "utils/jobcategorystrings.h"

static const char* error_texts[] = {
    nullptr,
    QT_TRANSLATE_NOOP("RsErrors", "Stop is transit. Cannot couple/uncouple rollingstock."),
    QT_TRANSLATE_NOOP("RsErrors", "Coupled while busy: it was already coupled to another job."),
    QT_TRANSLATE_NOOP("RsErrors", "Uncoupled when not coupled."),
    QT_TRANSLATE_NOOP("RsErrors", "Not uncoupled at the end of the job or coupled by another job before this jobs uncouples it."),
    QT_TRANSLATE_NOOP("RsErrors", "Coupled in a different station than that where it was uncoupled."),
    QT_TRANSLATE_NOOP("RsErrors", "Uncoupled in the same stop it was coupled.")
};

class RsError
{
    Q_DECLARE_TR_FUNCTIONS(RsErrors)
};

RsErrorTreeModel::RsErrorTreeModel(QObject *parent) :
    RsErrorTreeModelBase(parent)
{
    //FIXME: listen for changes in rs/job/station names and update them
}

QVariant RsErrorTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
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
        case Description:
            return tr("Description");
        default:
            break;
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

QVariant RsErrorTreeModel::data(const QModelIndex &idx, int role) const
{
    if(!idx.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const RsErrors::RSErrorData *item = getItem(idx);
    if(item)
    {
        switch (idx.column())
        {
        case JobName:
            return JobCategoryName::jobName(item->job.jobId, item->job.category);
        case StationName:
            return item->stationName;
        case Arrival:
            return item->time;
        case Description:
            return RsError::tr(error_texts[item->errorType]);
        }
    }
    else
    {
        //Caption
        if(idx.row() >= m_data.topLevelCount() || idx.column() != 0)
            return QVariant();

        auto topLevel = m_data.getTopLevelAtRow(idx.row());
        return topLevel->rsName;
    }

    return QVariant();
}

/* Description:
 * Clear current errors and set new ones
*/
void RsErrorTreeModel::setErrors(const QMap<db_id, RsErrors::RSErrorList> &data)
{
    beginResetModel();
    m_data.map = data;
    endResetModel();
}

/* Description:
 * Merge new errors with pre-existing.
 * - If an RS is passed with no errors (i.e. empty QVector) it gets removed from the model
 * - If an new RS is passed it gets inserted in the model
 * - If an RS already in the model is passed then its current errors are cleared and the new errors are inserted
*/
void RsErrorTreeModel::mergeErrors(const QMap<db_id, RsErrors::RSErrorList> &data)
{
    auto oldIter = m_data.map.begin();
    int row = 0;
    for(auto it = data.constBegin(); it != data.constEnd(); it++)
    {
        auto iter = m_data.map.find(it.key());
        if(iter == m_data.map.end()) //Insert a new RS
        {
            if(it->errors.isEmpty())
                continue; //Error: tried to remove an RS not in this model (maybe already removed)

            auto pos = m_data.map.lowerBound(it.key());
            row += std::distance(oldIter, pos);

            beginInsertRows(QModelIndex(), row, row);
            iter = m_data.map.insert(pos, it.key(), it.value());
            endInsertRows();
        }
        else
        {
            row += std::distance(oldIter, iter);

            if(it->errors.isEmpty()) //Remove RS
            {
                beginRemoveRows(QModelIndex(), row, row);
                iter = m_data.map.erase(iter);
                endRemoveRows();
            }
            else //Repopulate
            {
                //First remove old rows to invalidate QModelIndex because they store a pointer to vector elements that will become dangling
                QModelIndex parent = createIndex(row, 0, nullptr);
                beginRemoveRows(parent, 0, iter->errors.size() - 1);
                iter->errors.clear();
                endRemoveRows();

                beginInsertRows(parent, 0, it->errors.size() - 1);
                iter.value() = it.value(); //Copy errors QVector
                endInsertRows();
            }
        }

        oldIter = iter;
    }
}

void RsErrorTreeModel::clear()
{
    beginResetModel();
    m_data.map.clear();
    endResetModel();
}

void RsErrorTreeModel::onRSInfoChanged(db_id rsId)
{
    Q_UNUSED(rsId)
    //Update top-level items that show RS names
    if(m_data.topLevelCount())
    {
        QModelIndex first = index(0, 0);
        QModelIndex last = index(m_data.topLevelCount(), 0);

        emit dataChanged(first, last);
    }
}

#endif // ENABLE_BACKGROUND_MANAGER
