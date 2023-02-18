#include "nextprevrsjobsmodel.h"

#include "utils/jobcategorystrings.h"
#include "utils/rs_utils.h"

#include <QFont>

#include <sqlite3pp/sqlite3pp.h>

NextPrevRSJobsModel::NextPrevRSJobsModel(sqlite3pp::database &db, QObject *parent) :
    QAbstractTableModel(parent),
    mDb(db)
{
}

QVariant NextPrevRSJobsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case JobIdCol:
            return tr("Job");
        case RsNameCol:
            return tr("RS item");
        case TimeCol:
            return tr("Time");
        default:
            break;
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

int NextPrevRSJobsModel::rowCount(const QModelIndex &p) const
{
    return p.isValid() ? 0 : m_data.size();
}

int NextPrevRSJobsModel::columnCount(const QModelIndex &p) const
{
    return p.isValid() ? 0 : NCols;
}

QVariant NextPrevRSJobsModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_data.size() || idx.column() >= NCols)
        return QVariant();

    const Item& item = m_data.at(idx.row());

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case JobIdCol:
        {
            if(item.otherJobCat == JobCategory::NCategories)
                return tr("Depot"); //Rollingstock item taken from/released to depot
            return JobCategoryName::jobName(item.otherJobId, item.otherJobCat);
        }
        case RsNameCol:
            return item.rsName;
        case TimeCol:
            return item.opTime;
        }
        break;
    }
    case Qt::FontRole:
    {
        if(idx.column() == JobIdCol && item.otherJobCat == JobCategory::NCategories)
        {
            //Rollingstock item taken from/released to depot, distinguish font from job names
            QFont f;
            f.setItalic(true);
            return f;
        }
        break;
    }
    default:
        break;
    }

    return QVariant();
}

void NextPrevRSJobsModel::refreshData()
{
    //GROUP by rollingstock
    QByteArray sql = "SELECT c.id, c.rs_id, rs_list.number, rs_models.name, rs_models.suffix, rs_models.type, stops.%time0,"
                     " %min(s1.%time0), stops.id, s1.job_id AS job_id, j1.category"
                     " FROM stops"
                     " JOIN coupling c ON c.stop_id=stops.id"
                     " JOIN rs_list ON rs_list.id=c.rs_id"
                     " JOIN rs_models ON rs_models.id=rs_list.model_id"
                     " LEFT JOIN coupling c1 ON c1.rs_id=c.rs_id"
                     " LEFT JOIN stops s1 ON s1.id=c1.stop_id"
                     " LEFT JOIN jobs j1 ON j1.id=s1.job_id"
                     " WHERE stops.job_id=?1 AND c.operation=%rem AND c1.operation=%add AND s1.%time0 %gt stops.%time1"
                     " GROUP BY c.rs_id"
                     " UNION ALL "
                     "SELECT c.id, c.rs_id, rs_list.number, rs_models.name, rs_models.suffix, rs_models.type, stops.%time0,"
                     " %max(s1.%time0), stops.id, NULL AS job_id, NULL"
                     " FROM stops"
                     " JOIN coupling c ON c.stop_id=stops.id"
                     " JOIN rs_list ON rs_list.id=c.rs_id"
                     " JOIN rs_models ON rs_models.id=rs_list.model_id"
                     " LEFT JOIN coupling c1 ON c1.rs_id=c.rs_id"
                     " LEFT JOIN stops s1 ON s1.id=c1.stop_id"
                     " WHERE stops.job_id=?1 AND c.operation=%rem AND c1.operation=%add"
                     " GROUP BY c.rs_id"
                     " HAVING s1.%time1 %lt stops.%time0"
                     " ORDER BY stops.%time0, job_id, rs_models.type, rs_models.name, rs_list.number"
                     " LIMIT 100";

    if(m_mode == NextJobs)
    {
        sql.replace("%time0", "arrival");
        sql.replace("%time1", "departure");
        sql.replace("%min", "MIN");
        sql.replace("%max", "MAX");
        sql.replace("%rem", "0");
        sql.replace("%add", "1");
        sql.replace("%gt", ">=");
        sql.replace("%lt", "<=");
    }
    else
    {
        //Invert all
        sql.replace("%time0", "departure");
        sql.replace("%time1", "arrival");
        sql.replace("%min", "MAX");
        sql.replace("%max", "MIN");
        sql.replace("%rem", "1");
        sql.replace("%add", "0");
        sql.replace("%gt", "<=");
        sql.replace("%lt", ">=");
    }

    /*
    //GROUP by next job_id (NOTE: added COUNT(1) column)
    QByteArray sql1 = "SELECT c.id, c.rs_id, c.operation, rs_models.name, rs_list.number,"
                      " stops.arrival, MIN(s1.arrival), s1.job_id, c1.operation, COUNT(1)"
                      " FROM stops"
                      " JOIN coupling c ON c.stop_id=stops.id"
                      " JOIN rs_list ON rs_list.id=c.rs_id"
                      " JOIN rs_models ON rs_models.id=rs_list.model_id"
                      " LEFT JOIN coupling c1 ON c1.rs_id=c.rs_id"
                      " LEFT JOIN stops s1 ON s1.id=c1.stop_id"
                      " WHERE stops.job_id=16816 AND c.operation=0 AND c1.operation=1"
                      " AND s1.arrival >= stops.departure"
                      " GROUP BY s1.job_id"
                      " UNION ALL "
                      "SELECT *, COUNT(1) FROM (" //TODO: better way than sub query?
                      " SELECT c.id, c.rs_id, c.operation, rs_models.name, rs_list.number,"
                      " stops.arrival, MAX(s1.arrival), NULL, c1.operation, COUNT(1)"
                      " FROM stops"
                      " JOIN coupling c ON c.stop_id=stops.id"
                      " JOIN rs_list ON rs_list.id=c.rs_id"
                      " JOIN rs_models ON rs_models.id=rs_list.model_id"
                      " LEFT JOIN coupling c1 ON c1.rs_id=c.rs_id"
                      " LEFT JOIN stops s1 ON s1.id=c1.stop_id"
                      " WHERE stops.job_id=16816 AND c.operation=0 AND c1.operation=1"
                      " GROUP BY c.rs_id"
                      " HAVING s1.departure <= stops.arrival"
                      ")";
    */

    sqlite3pp::query q(mDb, sql);
    q.bind(1, m_jobId);

    beginResetModel();
    m_data.clear();

    for(auto row : q)
    {
        Item item;
        item.couplingId = row.get<db_id>(0);
        item.rsId = row.get<db_id>(1);

        int number = row.get<int>(2);
        int modelNameLen = sqlite3_column_bytes(q.stmt(), 3);
        const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(q.stmt(), 3));

        int modelSuffixLen = sqlite3_column_bytes(q.stmt(), 4);
        const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(q.stmt(), 4));
        RsType rsType = RsType(row.get<int>(5));

        item.rsName = rs_utils::formatNameRef(modelName,
                                              modelNameLen,
                                              number,
                                              modelSuffix,
                                              modelSuffixLen,
                                              rsType);

        item.opTime = row.get<QTime>(6);
        //Ignore column 7, which is used just for MIN/MAX
        item.stopId = row.get<db_id>(8);
        item.otherJobId = row.get<db_id>(9);
        item.otherJobCat = JobCategory(row.get<int>(10));
        if(row.column_type(10) == SQLITE_NULL)
            item.otherJobCat = JobCategory::NCategories;

        m_data.append(item);
    }

    endResetModel();
}

void NextPrevRSJobsModel::clearData()
{
    beginResetModel();
    m_data.clear();
    m_data.squeeze();
    endResetModel();
}

db_id NextPrevRSJobsModel::jobId() const
{
    return m_jobId;
}

void NextPrevRSJobsModel::setJobId(db_id newJobId)
{
    m_jobId = newJobId;
    if(m_jobId)
        refreshData();
    else
        clearData();
}

NextPrevRSJobsModel::Mode NextPrevRSJobsModel::mode() const
{
    return m_mode;
}

void NextPrevRSJobsModel::setMode(Mode newMode)
{
    m_mode = newMode;
    if(m_jobId)
        refreshData();
}

NextPrevRSJobsModel::Item NextPrevRSJobsModel::getItemAtRow(int row) const
{
    return m_data.value(row, Item());
}
