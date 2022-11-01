#include "e656stationmodel.h"

#include "utils/jobcategorystrings.h"

JobCategory matchCategory(const QString& name)
{
    if(name.contains(QLatin1String("FRECCIA"), Qt::CaseInsensitive) ||
        name.contains(QLatin1String("NTV"), Qt::CaseInsensitive))
    {
        return JobCategory::HIGH_SPEED;
    }

    if(name.contains(QLatin1String("REGIONALE"), Qt::CaseInsensitive))
    {
        return JobCategory::REGIONAL;
    }

    return JobCategory::NCategories;
}

E656StationModel::E656StationModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant E656StationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case JobCatName:
            return tr("Category");
        case JobNumber:
            return tr("Number");
        case JobMatchCat:
            return tr("Match");
        case PlatfCol:
            return tr("Platform");
        case ArrivalCol:
            return tr("Arrival");
        case DepartureCol:
            return tr("Departure");
        case OriginCol:
            return tr("Origin");
        case DestinationCol:
            return tr("Destination");
        default:
            break;
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

int E656StationModel::rowCount(const QModelIndex &p) const
{
    return p.isValid() ? 0 : m_data.size();
}

int E656StationModel::columnCount(const QModelIndex &p) const
{
    return p.isValid() ? 0 : NCols;
}

QVariant E656StationModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_data.size() || idx.column() >= NCols)
        return QVariant();

    const ImportedJobItem& item = m_data.at(idx.row());

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case JobCatName:
            return item.catName;
        case JobNumber:
            return item.number;
        case JobMatchCat:
            return JobCategoryName::shortName(item.matchingCategory);
        case PlatfCol:
            return item.platformName;
        case ArrivalCol:
            return item.arrival;
        case DepartureCol:
            return item.departure;
        case OriginCol:
            return item.originStation;
        case DestinationCol:
            return item.destinationStation;
        default:
            break;
        }
        break;
    }
    case Qt::ToolTipRole:
    {
        switch (idx.column())
        {
        case JobNumber:
            return item.urlPath;
        case JobMatchCat:
            return JobCategoryName::fullName(item.matchingCategory);
        case OriginCol:
            return item.originTime;
        case DestinationCol:
            return item.destinationTime;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    return QVariant();
}

void E656StationModel::setJobs(const QVector<ImportedJobItem> &jobs)
{
    beginResetModel();
    m_data = jobs;
    endResetModel();
}
