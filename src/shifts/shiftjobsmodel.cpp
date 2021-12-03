#include "shiftjobsmodel.h"

#include "utils/jobcategorystrings.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

ShiftJobsModel::ShiftJobsModel(sqlite3pp::database &db, QObject *parent) :
    QAbstractTableModel(parent),
    mDb(db)
{
}

QVariant ShiftJobsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case JobName:
            return tr("Job");
        case Departure:
            return tr("Departure");
        case Origin:
            return tr("Origin");
        case Arrival:
            return tr("Arrival");
        case Destination:
            return tr("Destination");
        default:
            break;
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
    }

int ShiftJobsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int ShiftJobsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant ShiftJobsModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_data.size() || idx.column() >= NCols || role != Qt::DisplayRole)
        return QVariant();

    const ShiftJobItem& item = m_data.at(idx.row());

    switch (idx.column())
    {
    case JobName:
        return JobCategoryName::jobName(item.jobId, item.cat);
    case Departure:
        return item.start;
    case Origin:
        return item.originStName;
    case Arrival:
        return item.end;
    case Destination:
        return item.desinationStName;
    }

    return QVariant();
}

void ShiftJobsModel::loadShiftJobs(db_id shiftId)
{
    beginResetModel();

    m_data.clear();

    query q(mDb, "SELECT jobs.id,"
                 " jobs.category,"
                 " s1.arrival, s1.station_id,"
                 " s2.departure, s2.station_id,"
                 " st1.name,st2.name,"
                 " MIN(s1.arrival), MAX(s1.departure)"
                 " FROM jobs"
                 " JOIN stops s1 ON s1.job_id=jobs.id"
                 " JOIN stops s2 ON s2.job_id=jobs.id"
                 " JOIN stations st1 ON st1.id=s1.station_id"
                 " JOIN stations st2 ON st2.id=s2.station_id"
                 " WHERE jobs.shift_id=?"
                 " GROUP BY jobs.id"
                 " ORDER BY s1.arrival ASC");

    q.bind(1, shiftId);

    for(auto r : q)
    {
        ShiftJobItem item;
        item.jobId = r.get<db_id>(0);
        item.cat = JobCategory(r.get<int>(1));
        item.start = r.get<QTime>(2);
        item.originStId = r.get<db_id>(3);
        item.end = r.get<QTime>(4);
        item.destinationStId = r.get<db_id>(5);
        item.originStName = r.get<QString>(6);
        item.desinationStName = r.get<QString>(7);
        m_data.append(item);
    }

    m_data.squeeze();

    endResetModel();
}

db_id ShiftJobsModel::getJobAt(int row)
{
    if(row < m_data.size())
        return m_data.at(row).jobId;
    return 0;
}
