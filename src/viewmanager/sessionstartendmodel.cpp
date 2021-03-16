#include "sessionstartendmodel.h"

#include <QFont>

#include <sqlite3pp/sqlite3pp.h>

#include "utils/jobcategorystrings.h"

#include "utils/platform_utils.h"
#include "utils/rs_utils.h"

#include <QDebug>

static constexpr quintptr PARENT = quintptr(-1);

SessionStartEndModel::SessionStartEndModel(sqlite3pp::database &db, QObject *parent) :
    QAbstractItemModel(parent),
    mDb(db),
    m_mode(SessionRSMode::StartOfSession),
    m_order(SessionRSOrder::ByStation)
{

}

QVariant SessionStartEndModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
        case RsNameCol:
            return tr("Rollingstock");
        case JobCol:
            return tr("Job");
        case PlatfCol:
            return tr("Platform");
        case DepartureCol:
            return m_mode == SessionRSMode::StartOfSession ? tr("Departure") : tr("Arrival");
        case StationOrOwnerCol:
            return m_order == SessionRSOrder::ByStation ? tr("Owner") : tr("Station"); //Opposite of order
        default:
            break;
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

QModelIndex SessionStartEndModel::index(int row, int column, const QModelIndex &p) const
{
    if(p.isValid())
    {
        if(p.internalId() != PARENT)
            return QModelIndex();

        //It's a rs
        int parentRow = p.row();
        if(parentRow < 0 || parentRow >= parents.size())
            return QModelIndex();

        if(row < 0 || (row + parents[parentRow].firstIdx) >= rsData.size() || column < 0 || column >= NCols)
            return QModelIndex();

        QModelIndex par = parent(createIndex(row, column, parentRow));
        if(par != p)
        {
            qWarning() << "IDX ERROR" << p << par;
            par = parent(createIndex(row, column, parentRow));
        }

        return createIndex(row, column, parentRow);
    }

    //Station or RS owner
    if(row < 0 || row >= parents.size() || column != 0)
        return QModelIndex();

    Q_ASSERT(parent(createIndex(row, column, PARENT)) == p);

    return createIndex(row, column, PARENT);
}

QModelIndex SessionStartEndModel::parent(const QModelIndex &idx) const
{
    if(!idx.isValid() || idx.internalId() == PARENT)
        return QModelIndex();

    int parentRow = int(idx.internalId());

    return createIndex(parentRow, 0, PARENT);
}

int SessionStartEndModel::rowCount(const QModelIndex &p) const
{
    if(p.isValid())
    {
        if(p.internalId() != PARENT)
            return 0; //RS

        //Station or Owner
        int firstIdx = parents.at(p.row()).firstIdx;
        int lastIdx = rsData.size(); //Until end
        if(p.row() + 1 < parents.size())
            lastIdx = parents.at(p.row() + 1).firstIdx; //Until next station/owner first index
        return lastIdx - firstIdx;
    }

    //Root
    return parents.size();
}

int SessionStartEndModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 1 : NCols;
}

QVariant SessionStartEndModel::data(const QModelIndex &idx, int role) const
{
    if(!idx.isValid())
        return QVariant();

    if(idx.internalId() == PARENT)
    {
        //Station or Owner
        switch (role)
        {
        case Qt::DisplayRole:
        {
            return parents.at(idx.row()).name;
        }
        case Qt::FontRole:
        {
            QFont f;
            f.setBold(true);
            return f;
        }
        }
    }
    else
    {
        //RS
        int realIdx = parents.at(int(idx.internalId())).firstIdx + idx.row();
        const RSItem& item = rsData.at(realIdx);
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (idx.column())
            {
            case RsNameCol:
                return item.rsName;
            case JobCol:
                //return item.jobId;
                return JobCategoryName::jobName(item.jobId, item.jobCategory);
            case PlatfCol:
                return utils::platformName(item.platform);
            case DepartureCol:
                return item.time;
            case StationOrOwnerCol:
            {
                return item.stationOrOwnerName;
            }
            }
        }
        }
    }

    return QVariant();
}

bool SessionStartEndModel::hasChildren(const QModelIndex &parent) const
{
    if(parent.isValid())
    {
        if(parent.internalId() == PARENT)
            return true;
        return false;
    }

    return parents.size();
}

QModelIndex SessionStartEndModel::sibling(int row, int column, const QModelIndex &idx) const
{
    if(!idx.isValid())
        return QModelIndex();

    if(idx.internalId() == PARENT)
    {
        if(row < parents.size() && column == 1)
            return createIndex(row, column, PARENT);
    }
    else
    {
        int parentIdx = int(idx.internalId());
        int firstIdx = parents.at(parentIdx).firstIdx;
        int lastIdx = rsData.size();
        if(parentIdx + 1 < parents.size())
            lastIdx = parents.at(parentIdx + 1).firstIdx;

        int count = firstIdx - lastIdx;
        if(row < count)
            return createIndex(row, column, parentIdx);
    }

    return QModelIndex();
}

Qt::ItemFlags SessionStartEndModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags f;

    if(!idx.isValid())
        return f;

    f.setFlag(Qt::ItemIsEnabled);
    f.setFlag(Qt::ItemIsSelectable);

    if(idx.internalId() != PARENT)
        f.setFlag(Qt::ItemNeverHasChildren);

    return f;
}

void SessionStartEndModel::setMode(SessionRSMode m, SessionRSOrder o, bool forceReload)
{
    if(m_mode == m && m_order == o && !forceReload)
        return;

    beginResetModel();
    m_mode = m;
    m_order = o;

    rsData.clear();
    parents.clear();

    sqlite3pp::query q(mDb);

    q.prepare("SELECT COUNT(DISTINCT rsId) FROM coupling");
    q.step();
    int count = q.getRows().get<int>(0);
    q.reset();

    rsData.reserve(count);

    //TODO: fetch departure instead of arrival for start session

    //Query template: MIN/MAX to get RS at start/end of session then order by station/s_owner
    const auto sql = QStringLiteral("SELECT %1(stops.arrival), stops.stationId, rs_list.owner_id,"
                                    " stations.name, rs_owners.name,"
                                    " rs_list.id, rs_models.name, rs_models.suffix, rs_list.number, rs_models.type,"
                                    " stops.jobId, jobs.category,"
                                    " stops.platform, coupling.operation"
                                    " FROM rs_list"
                                    " JOIN coupling ON coupling.rsId=rs_list.id"
                                    " JOIN stops ON stops.id=coupling.stopId"
                                    " JOIN stations ON stations.id=stops.stationId"
                                    " JOIN jobs ON jobs.id=stops.jobId"
                                    " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id"
                                    " LEFT JOIN rs_owners ON rs_owners.id=rs_list.owner_id" //It might be null
                                    " GROUP BY rs_list.id"
                                    " ORDER BY %2, stops.arrival, stops.jobId, rs_list.model_id");

    QByteArray query = sql.arg(m_mode == SessionRSMode::StartOfSession ? "MIN" : "MAX")
            .arg(m_order == SessionRSOrder::ByStation ? "stops.stationId" : "rs_list.owner_id")
            .toUtf8();

    q.prepare(query.constData());

    db_id lastParentId = 0;
    int row = 0;

    sqlite3_stmt *stmt = q.stmt();
    for(auto rs : q)
    {
        RSItem item;
        item.time = rs.get<QTime>(0);

        db_id stationId = rs.get<db_id>(1);
        db_id ownerId = rs.get<db_id>(2);

        db_id parentId = 0;

        if(m_order == SessionRSOrder::ByStation)
        {
            parentId = stationId;
            item.stationOrOwnerId = ownerId;
            item.stationOrOwnerName = rs.get<QString>(4);
        }else{
            parentId = ownerId;
            item.stationOrOwnerId = stationId;
            item.stationOrOwnerName = rs.get<QString>(3);
        }

        if(parentId != lastParentId)
        {
            ParentItem s;
            s.id = parentId;
            s.name = rs.get<QString>(m_order == SessionRSOrder::ByStation ? 3 : 4);
            s.firstIdx = row;
            parents.append(s);

            lastParentId = parentId;
        }

        item.parentIdx = parents.size() - 1;
        item.rsId = rs.get<db_id>(5);

        int modelNameLen = sqlite3_column_bytes(stmt, 6);
        const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 6));

        int modelSuffixLen = sqlite3_column_bytes(stmt, 7);
        const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 7));
        int number = rs.get<int>(8);
        RsType type = RsType(rs.get<int>(9));

        item.rsName = rs_utils::formatNameRef(modelName, modelNameLen,
                                              number,
                                              modelSuffix, modelSuffixLen,
                                              type);

        item.jobId = rs.get<db_id>(10);
        item.jobCategory = JobCategory(rs.get<int>(11));

        item.platform = rs.get<int>(12);

        //TODO: check operation

        rsData.append(item);

        row++;
    }

    endResetModel();
}
