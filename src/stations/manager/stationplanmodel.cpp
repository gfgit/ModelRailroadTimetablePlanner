#include "stationplanmodel.h"

#include "utils/jobcategorystrings.h"

#include <QColor>

StationPlanModel::StationPlanModel(sqlite3pp::database &db, QObject *parent) :
    QAbstractTableModel(parent),
    mDb(db),
    q_countPlanItems(mDb, "SELECT COUNT(id) FROM stops WHERE station_id=?"),
    q_selectPlan(mDb, "SELECT stops.id,"
                      "stops.job_id,"
                      "jobs.category,"
                      "stops.arrival,"
                      "stops.departure,"
                      "stops.type,"
                      "t1.name,"
                      "t2.name"
                      " FROM stops"
                      " JOIN jobs ON jobs.id=stops.job_id"
                      " LEFT JOIN station_gate_connections g1 ON g1.id=stops.in_gate_conn"
                      " LEFT JOIN station_gate_connections g2 ON g2.id=stops.out_gate_conn"
                      " LEFT JOIN station_tracks t1 ON t1.id=g1.track_id"
                      " LEFT JOIN station_tracks t2 ON t2.id=g2.track_id"
                      " WHERE stops.station_id=?"
                      " ORDER BY stops.arrival,stops.job_id")
{
}

QVariant StationPlanModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case Arrival:
            return tr("Arrival");
        case Departure:
            return tr("Departure");
        case Platform:
            return tr("Platform");
        case Job:
            return tr("Job");
        case Notes:
            return tr("Notes");
        default:
            break;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int StationPlanModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int StationPlanModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant StationPlanModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_data.size() || idx.column() >= NCols)
        return QVariant();

    const StPlanItem& item = m_data.at(idx.row());

    switch (role)
    {
    case Qt::DisplayRole:
    {

        switch (idx.column())
        {
        case Arrival:
        {
            if(item.type == StPlanItem::ItemType::Departure)
                break; //Don't repeat arrival also in the second row.
            return item.arrival;
        }
        case Departure:
            return item.departure;
        case Platform:
            return item.platform;
        case Job:
            return JobCategoryName::jobName(item.jobId, item.cat);
        case Notes:
        {
            if(item.type == StPlanItem::ItemType::Departure)
                return StationPlanModel::tr("Departure"); //Don't repeat description also in the second row.

            return item.description;
        }
        }
        break;
    }
    case Qt::ForegroundRole:
    {
        if(item.type == StPlanItem::ItemType::Transit)
            return QColor(0, 0, 255); //Transit in blue
        break;
    }
    case Qt::ToolTipRole:
    {
        if(item.type == StPlanItem::ItemType::Transit)
            return StationPlanModel::tr("Transit");
        break;
    }
    }

    return QVariant();
}

void StationPlanModel::clear()
{
    beginResetModel();
    m_data.clear();
    m_data.squeeze();
    endResetModel();
}

void StationPlanModel::loadPlan(db_id stId)
{
    beginResetModel();

    m_data.clear();

    q_countPlanItems.bind(1, stId);
    q_countPlanItems.step();
    int count = q_countPlanItems.getRows().get<int>(0);
    q_countPlanItems.reset();

    if(count > 0)
    {
        QMap<QTime, StPlanItem> stopMap; //Order by Departure ASC
        m_data.reserve(count);

        q_selectPlan.bind(1, stId);
        for(auto r : q_selectPlan)
        {
            StPlanItem curStop;
            curStop.stopId = r.get<db_id>(0);
            curStop.jobId = r.get<db_id>(1);
            curStop.cat = JobCategory(r.get<int>(2));
            curStop.arrival = r.get<QTime>(3);
            curStop.departure = r.get<QTime>(4);
            curStop.type = StPlanItem::ItemType::Normal;
            StopType stopType = StopType(r.get<int>(5));

            curStop.platform = r.get<QString>(6);
            if(curStop.platform.isEmpty())
                curStop.platform = r.get<QString>(7); //Use out gate to get track name

            for(auto stop = stopMap.begin(); stop != stopMap.end(); /*nothing because of erase */)
            {
                if(stop->departure <= curStop.arrival)
                {
                    m_data.append(stop.value());
                    m_data.last().type = StPlanItem::ItemType::Departure;
                    stop = stopMap.erase(stop);
                }
                else
                {
                    ++stop;
                }
            }

            m_data.append(curStop);
            if(stopType == StopType::Transit)
            {
                m_data.last().type = StPlanItem::ItemType::Transit;
            }
            else
            {
                //Transit should not be repeated
                curStop.type = StPlanItem::ItemType::Departure;
                stopMap.insert(curStop.departure, std::move(curStop));
            }
        }
        q_selectPlan.reset();

        for(const StPlanItem& stop : stopMap)
        {
            m_data.append(stop);
        }
    }

    m_data.squeeze();

    endResetModel();
}

std::pair<db_id, db_id> StationPlanModel::getJobAndStopId(int row) const
{
    if(row < m_data.size())
    {
        const StPlanItem& item = m_data.at(row);
        return {item.jobId, item.stopId};
    }
    return {0, 0};
}
