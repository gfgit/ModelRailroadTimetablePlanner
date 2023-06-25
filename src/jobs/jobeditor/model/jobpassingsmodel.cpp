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

#include "jobpassingsmodel.h"

#include <QFont>

#include "utils/jobcategorystrings.h"

JobPassingsModel::JobPassingsModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

QVariant JobPassingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case JobNameCol:
        {
            return tr("Job");
        }
        case ArrivalCol:
        {
            return tr("Arrival");
        }
        case DepartureCol:
        {
            return tr("Departure");
        }
        case PlatformCol:
        {
            return tr("Platform");
        }
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

int JobPassingsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int JobPassingsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 4;
}

QVariant JobPassingsModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_data.size() || idx.column() >= NCols)
        return QVariant();

    const Entry &e = m_data.at(idx.row());
    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case JobNameCol:
            return JobCategoryName::jobName(e.jobId, e.category);
        case ArrivalCol:
            return e.arrival;
        case DepartureCol:
            return e.departure;
        case PlatformCol:
            return e.platform;
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        return Qt::AlignCenter;
    }
    case Qt::FontRole:
    {
        QFont f;
        f.setPointSize(10);
        if (idx.column() == JobNameCol)
            f.setBold(true);
        return f;
    }
    }
    return QVariant();
}

void JobPassingsModel::setJobs(const QVector<JobPassingsModel::Entry> &vec)
{
    beginResetModel();
    m_data = vec;
    endResetModel();
}
