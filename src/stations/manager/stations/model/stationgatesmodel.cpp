#include "stationgatesmodel.h"

#include "utils/delegates/sql/pageditemmodelhelper_impl.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "stations/station_name_utils.h"

#include <QDebug>

//Error messages
static constexpr char
    errorNameNotLetterText[] = QT_TRANSLATE_NOOP("StationGatesModel",
                        "Gate name must be a letter.");

static constexpr char
    errorNameAlreadyUsedText[] = QT_TRANSLATE_NOOP("StationGatesModel",
                        "This gate letter <b>%1</b> is already used by another gate of this station.");

static constexpr char
    errorGateInUseText[] = QT_TRANSLATE_NOOP("StationGatesModel",
                        "This gate is still referenced.\n"
                        "Please remove all references before deleting it.");

static constexpr char
    errorGateEntranceOrExitText[] = QT_TRANSLATE_NOOP("StationGatesModel",
                        "The gate must be at least Entrance or Exit.\n"
                        "It can also be both (Bidirectional) but cannot be neither.");


StationGatesModel::StationGatesModel(sqlite3pp::database &db, QObject *parent) :
    BaseClass(BatchSize, db, parent),
    m_stationId(0),
    editable(false)
{
    sortColumn = LetterCol;
}

QVariant StationGatesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (section)
            {
            case LetterCol:
                return tr("Name");
            case SideCol:
                return tr("Side");
            case OutTrackCountCol:
                return tr("Out Track Count");
            case DefaultInPlatfCol:
                return tr("Def. In platf.");
            case IsEntranceCol:
                return tr("Entrance");
            case IsExitCol:
                return tr("Exit");
            }
            break;
        }
        case Qt::ToolTipRole:
        {
            switch (section)
            {
            case OutTrackCountCol:
                return tr("Number of tracks going out from this gate");
            case DefaultInPlatfCol:
                return tr("Default platform in which trains stop if they arrive from this gate.");
            case IsEntranceCol:
                return tr("Entrance");
            case IsExitCol:
                return tr("Exit");
            }
            break;
        }
        }
    }
    else if(role == Qt::DisplayRole)
    {
        return section + curPage * ItemsPerPage + 1;
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant StationGatesModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<StationGatesModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const GateItem& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case LetterCol:
            return item.letter;
        case SideCol:
            return utils::StationUtils::name(item.side);
        case OutTrackCountCol:
            return item.outTrackCount;
        case DefaultInPlatfCol:
            return item.defPlatfName;
        }
        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case LetterCol:
            return item.letter;
        case SideCol:
            return int(item.side);
        case OutTrackCountCol:
            return item.outTrackCount;
        case DefaultInPlatfCol:
            return item.defaultInPlatfId;
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        switch (idx.column())
        {
        case LetterCol:
            return Qt::AlignCenter;
        case OutTrackCountCol:
        case DefaultInPlatfCol:
            return Qt::AlignRight + Qt::AlignVCenter;
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        switch (idx.column())
        {
        case IsEntranceCol:
            return item.type.testFlag(utils::GateType::Entrance) ? Qt::Checked : Qt::Unchecked;
        case IsExitCol:
            return item.type.testFlag(utils::GateType::Exit) ? Qt::Checked : Qt::Unchecked;
        }
        break;
    }
    }

    return QVariant();
}

bool StationGatesModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if(!editable)
        return false;

    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    GateItem &item = cache[row - cacheFirstRow];
    QModelIndex first = idx;
    QModelIndex last = idx;

    switch (role)
    {
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case LetterCol:
        {
            QString str = value.toString().simplified();
            if(str.isEmpty() || !setName(item, str.at(0)))
                return false;
            break;
        }
        case SideCol:
        {
            bool ok = false;
            int val = value.toInt(&ok);
            if(!ok || !setSide(item, val))
                return false;
            break;
        }
        case OutTrackCountCol:
        {
            bool ok = false;
            int val = value.toInt(&ok);
            if(!ok || !setOutTrackCount(item, val))
                return false;
            break;
        }
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        switch (idx.column())
        {
        case IsEntranceCol:
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            QFlags<utils::GateType> type = item.type;
            type.setFlag(utils::GateType::Entrance, cs == Qt::Checked);

            if(!setType(item, type))
                return false;
            break;
        }
        case IsExitCol:
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            QFlags<utils::GateType> type = item.type;
            type.setFlag(utils::GateType::Exit, cs == Qt::Checked);

            if(!setType(item, type))
                return false;
            break;
        }
        }
        break;
    }
    }

    emit dataChanged(first, last);

    return true;
}

Qt::ItemFlags StationGatesModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    if(idx.column() == IsEntranceCol || idx.column() == IsExitCol)
        f.setFlag(Qt::ItemIsUserCheckable);
    else if(editable)
        f.setFlag(Qt::ItemIsEditable);

    return f;
}

void StationGatesModel::setSortingColumn(int col)
{
    if(sortColumn == col || (col != LetterCol && col != SideCol))
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

bool StationGatesModel::getFieldData(int row, int col, db_id &idOut, QString &nameOut) const
{
    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size() || col != DefaultInPlatfCol)
        return false;

    const GateItem& item = cache[row - cacheFirstRow];
    idOut = item.defaultInPlatfId;
    nameOut = item.defPlatfName;

    return true;
}

bool StationGatesModel::validateData(int row, int col, db_id id, const QString &name)
{
    Q_UNUSED(row)
    Q_UNUSED(col)
    Q_UNUSED(id)
    Q_UNUSED(name)
    return true;
}

bool StationGatesModel::setFieldData(int row, int col, db_id id, const QString &name)
{
    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size() || col != DefaultInPlatfCol)
        return false;

    GateItem& item = cache[row - cacheFirstRow];
    if(setDefaultPlatf(item, id, name))
    {
        QModelIndex idx = index(row, DefaultInPlatfCol);
        emit dataChanged(idx, idx);
        return true;
    }
    return false;
}

bool StationGatesModel::setStation(db_id stationId)
{
    m_stationId = stationId;
    refreshData(true);
    return true;
}

bool StationGatesModel::addGate(const QChar &name, db_id *outGateId)
{
    char ch = name.toUpper().toLatin1();
    if(ch < 'A' || ch > 'Z')
    {
        emit modelError(tr(errorNameNotLetterText));
        return false;
    }

    char tmp[2] = { ch, '\0'};
    const utils::GateType type = utils::GateType::Bidirectional;

    command q_newGate(mDb, "INSERT INTO station_gates"
                           "(id, station_id, out_track_count, type, def_in_platf_id, name, side)"
                           " VALUES (NULL, ?, 1, ?, NULL, ?, 0)");
    q_newGate.bind(1, m_stationId);
    q_newGate.bind(2, int(type));
    q_newGate.bind(3, tmp, sqlite3pp::copy_semantic::nocopy);

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret = q_newGate.execute();
    db_id gateId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_newGate.reset();

    if((ret != SQLITE_OK && ret != SQLITE_DONE) || gateId == 0)
    {
        //Error
        if(outGateId)
            *outGateId = 0;

        if(ret == SQLITE_CONSTRAINT_UNIQUE)
        {
            emit modelError(tr(errorNameAlreadyUsedText).arg(ch));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }
        return false;
    }

    if(outGateId)
        *outGateId = gateId;

    refreshData(); //Recalc row count
    setSortingColumn(LetterCol);
    switchToPage(0); //Reset to first page and so it is shown as first row

    return true;
}

bool StationGatesModel::removeGate(db_id gateId)
{
    command q(mDb, "DELETE FROM station_gates WHERE id=?");

    q.bind(1, gateId);
    int ret = q.execute();
    q.reset();

    if(ret != SQLITE_OK)
    {
        if(ret == SQLITE_CONSTRAINT_TRIGGER)
        {
            //TODO: show more information to the user, like where it's still referenced
            emit modelError(tr(errorGateInUseText));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }

        return false;
    }

    refreshData(); //Recalc row count

    emit gateRemoved(gateId);

    return true;
}

QString StationGatesModel::getStationName() const
{
    query q(mDb, "SELECT name FROM stations WHERE id=?");
    q.bind(1, m_stationId);
    if(q.step() != SQLITE_ROW)
        return QString();
    return q.getRows().get<QString>(0);
}

bool StationGatesModel::getStationInfo(QString& name, QString &shortName,
                                       utils::StationType &type, qint64 &phoneNumber,
                                       bool &hasImage) const
{
    query q(mDb, "SELECT name,short_name,type,phone_number,svg_data IS NULL FROM stations WHERE id=?");
    q.bind(1, m_stationId);
    if(q.step() != SQLITE_ROW)
        return false;
    auto r = q.getRows();
    name = r.get<QString>(0);
    shortName = r.get<QString>(1);
    type = utils::StationType(r.get<int>(2));

    if(r.column_type(3) == SQLITE_NULL)
        phoneNumber = -1;
    else
        phoneNumber = r.get<qint64>(3);

    //svg_data IS NULL == 0 --> station has SVG image
    hasImage = r.get<int>(4) == 0;

    return true;
}

bool StationGatesModel::setStationInfo(const QString &name, const QString &shortName, utils::StationType type, qint64 phoneNumber)
{
    command q(mDb, "UPDATE stations SET name=?,short_name=?,type=?,phone_number=? WHERE id=?");
    q.bind(1, name);
    if(shortName.isEmpty())
        q.bind(2); //Bind NULL
    else
        q.bind(2, shortName);
    q.bind(3, int(type));
    if(phoneNumber == -1)
        q.bind(4); //Bind NULL
    else
        q.bind(4, phoneNumber);
    q.bind(5, m_stationId);
    return q.execute() == SQLITE_OK;
}

qint64 StationGatesModel::recalcTotalItemCount()
{
    //TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM station_gates WHERE station_id=?");
    q.bind(1, m_stationId);
    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void StationGatesModel::internalFetch(int first, int sortCol, int valRow, const QVariant &val)
{
    query q(mDb);

    int offset = first - valRow + curPage * ItemsPerPage;
    bool reverse = false;

    if(valRow > first)
    {
        offset = 0;
        reverse = true;
    }

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    const char *whereCol = nullptr;

    QByteArray sql = "SELECT g.id,g.out_track_count,g.type,g.def_in_platf_id,g.name,g.side,t.name FROM station_gates g"
                     " LEFT JOIN station_tracks t ON t.id=g.def_in_platf_id";
    switch (sortCol)
    {
    case LetterCol:
    {
        whereCol = "g.name"; //Order by 1 column, no where clause
        break;
    }
    case SideCol:
    {
        whereCol = "g.side,g.name";
        break;
    }
    }

//    if(val.isValid())
//    {
//        sql += " WHERE ";
//        sql += whereCol;
//        if(reverse)
//            sql += "<?3";
//        else
//            sql += ">?3";
//    }
    sql += " WHERE g.station_id=?4";

    sql += " ORDER BY ";
    sql += whereCol;

    if(reverse)
        sql += " DESC";

    sql += " LIMIT ?1";
    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);

    q.bind(4, m_stationId);

    //    if(val.isValid())
    //    {
    //        switch (sortCol)
    //        {
    //        case LineNameCol:
    //        {
    //            q.bind(3, val.toString());
    //            break;
    //        }
    //        }
    //    }

    QVector<GateItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    int i = reverse ? BatchSize - 1 : 0;
    const int increment = reverse ? -1 : 1;

    for(; it != end; ++it)
    {
        auto r = *it;
        GateItem &item = vec[i];
        item.gateId = r.get<db_id>(0);
        item.outTrackCount = r.get<int>(1);
        item.type = utils::GateType(r.get<int>(2));
        item.defaultInPlatfId = r.get<db_id>(3);
        item.letter = r.get<const char *>(4)[0];
        item.side = utils::Side(r.get<int>(5));
        if(r.column_type(6) != SQLITE_NULL)
            item.defPlatfName = r.get<QString>(6);

        i += increment;
    }

    if(reverse && i > -1)
        vec.remove(0, i + 1);
    else if(i < BatchSize)
        vec.remove(i, BatchSize - i);

    postResult(vec, first);
}

bool StationGatesModel::setName(StationGatesModel::GateItem &item, const QChar &val)
{
    char ch = val.toUpper().toLatin1();
    if(ch < 'A' || ch > 'Z')
    {
        emit modelError(tr(errorNameNotLetterText));
        return false;
    }

    if(item.letter == ch)
        return false;

    char tmp[2] = { ch, '\0'};

    command q(mDb, "UPDATE station_gates SET name=? WHERE id=?");
    q.bind(1, tmp, sqlite3pp::copy_semantic::nocopy);
    q.bind(2, item.gateId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        if(ret == SQLITE_CONSTRAINT_UNIQUE)
        {
            emit modelError(tr(errorNameAlreadyUsedText).arg(ch));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }
        return false;
    }

    item.letter = ch;
    emit gateNameChanged(item.gateId, item.letter);

    //This row has now changed position so we need to invalidate cache
    //HACK: we emit dataChanged for this index (that doesn't exist anymore)
    //but the view will trigger fetching at same scroll position so it is enough
    cache.clear();
    cacheFirstRow = 0;

    return true;
}

bool StationGatesModel::setSide(StationGatesModel::GateItem &item, int val)
{
    utils::Side side = utils::Side(val);
    if(item.side == side)
        return false;

    command q(mDb, "UPDATE station_gates SET side=? WHERE id=?");
    q.bind(1, val);
    q.bind(2, item.gateId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        return false;
    }

    item.side = side;

    if(sortColumn == SideCol)
    {
        //This row has now changed position so we need to invalidate cache
        //HACK: we emit dataChanged for this index (that doesn't exist anymore)
        //but the view will trigger fetching at same scroll position so it is enough
        cache.clear();
        cacheFirstRow = 0;
    }

    return true;
}

bool StationGatesModel::setType(StationGatesModel::GateItem &item, QFlags<utils::GateType> type)
{
    if(item.type == type)
        return false;

    if((type & utils::GateType::Bidirectional) == 0)
    {
        //Gate is neither Entrance nor Exit
        //This is also checked by SQL CHECK() but this error message is better
        emit modelError(tr(errorGateEntranceOrExitText));
        return false;
    }

    command q(mDb, "UPDATE station_gates SET type=? WHERE id=?");
    q.bind(1, type);
    q.bind(2, item.gateId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        return false;
    }

    item.type = type;

    return true;
}

bool StationGatesModel::setOutTrackCount(StationGatesModel::GateItem &item, int count)
{
    if(item.outTrackCount == count)
        return false;

    //FIXME: check if there are railway_connections referencing track higher than count - 1.
    //Like connecting a double track line and then setting track count to 1 -> Error
    //Message box offering to delete track connection or leave old track count

    command q(mDb, "UPDATE station_gates SET out_track_count=? WHERE id=?");
    q.bind(1, count);
    q.bind(2, item.gateId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        return false;
    }

    item.outTrackCount = count;

    return true;
}

bool StationGatesModel::setDefaultPlatf(StationGatesModel::GateItem &item, db_id trackId, const QString& trackName)
{
    if(item.defaultInPlatfId == trackId)
        return false;

    //FIXME: restrict popup suggestions to tracks connected to this gate and not all station's tracks

    command q(mDb, "UPDATE station_gates SET def_in_platf_id=? WHERE id=?");
    if(trackId)
        q.bind(1, trackId);
    else
        q.bind(1); //Bind NULL
    q.bind(2, item.gateId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        return false;
    }

    item.defaultInPlatfId = trackId;
    item.defPlatfName = trackName;

    return true;
}
