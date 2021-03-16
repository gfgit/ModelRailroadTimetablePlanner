#include "jobpassingsmodel.h"

#include <QFont>

#include "utils/model_roles.h"
#include "utils/jobcategorystrings.h"
#include "utils/platform_utils.h"

JobPassingsModel::JobPassingsModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

QVariant JobPassingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
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

    const Entry& e = m_data.at(idx.row());
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
            return utils::platformName(e.platform);
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
        if(idx.column() == JobNameCol)
            f.setBold(true);
        return f;
    }
    case JOB_ID_ROLE:
    {
        return e.jobId;
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
