#include "e656stationmodel.h"

#include "utils/jobcategorystrings.h"

#include "../e656netimporter.h"
#include <QNetworkReply>

E656StationModel::E656StationModel(sqlite3pp::database &db, QObject *parent) :
    QAbstractTableModel(parent)
{

}

E656StationModel::~E656StationModel()
{

}

QVariant E656StationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case JobCatNameCol:
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
        case JobCatNameCol:
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
    case Qt::CheckStateRole:
    {
        if(idx.column() == JobCatNameCol && item.status != ImportedJobItem::AlreadyImported)
        {
            return item.status == ImportedJobItem::ToBeImported ? Qt::Checked : Qt::Unchecked;
        }
        break;
    }
    default:
        break;
    }

    return QVariant();
}

bool E656StationModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if(role != Qt::CheckStateRole || !idx.isValid() || idx.column() > 0 || idx.row() >= m_data.size())
        return false;

    Qt::CheckState state = value.value<Qt::CheckState>();

    ImportedJobItem& item = m_data[idx.row()];
    if(item.status == ImportedJobItem::AlreadyImported)
        return false;

    item.status = state == Qt::Checked ? ImportedJobItem::ToBeImported : ImportedJobItem::Unselected;
    return true;
}

Qt::ItemFlags E656StationModel::flags(const QModelIndex &idx) const
{
    if(!idx.isValid() || idx.column() >= NCols || idx.row() >= m_data.size())
        return Qt::NoItemFlags;

    const ImportedJobItem& item = m_data.at(idx.row());
    Qt::ItemFlags f = Qt::ItemIsEnabled
                      | Qt::ItemNeverHasChildren
                      | Qt::ItemIsSelectable;

    if(idx.column() == JobCatNameCol && item.status != ImportedJobItem::AlreadyImported)
        f.setFlag(Qt::ItemIsUserCheckable);

    return f;
}

void E656StationModel::setJobs(const QVector<ImportedJobItem> &jobs)
{
    beginResetModel();
    m_data = jobs;
    endResetModel();
}

void E656StationModel::importSelectedJobs(E656NetImporter *importer)
{
    QUrl url;
    url.setScheme(QLatin1String("https"));
    url.setHost(QLatin1String("www.e656.net"));

    int counter = 0;
    for(ImportedJobItem& job : m_data)
    {
        if(counter > 10)
            break;

        if(job.status != ImportedJobItem::ToBeImported)
            continue;

        counter++;
        job.status = ImportedJobItem::AlreadyImported;

        url.setPath(job.urlPath);
        QNetworkReply *reply = importer->startImportJob(url);
        auto onFinished = [importer, reply, job]()
        {
            reply->deleteLater();

            if(reply->error() != QNetworkReply::NoError)
            {
                qDebug() << "Error:" << reply->errorString();
                return;
            }

            importer->handleJobReply(reply, job);
        };

        connect(reply, &QNetworkReply::finished, this, onFinished);
    }

    emit dataChanged(index(0, 0), index(m_data.size() - 1, NCols - 1));
}
