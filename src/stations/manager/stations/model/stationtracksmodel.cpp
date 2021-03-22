#include "stationtracksmodel.h"

#include <QCoreApplication>
#include <QEvent>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/worker_event_types.h"

#include "utils/model_roles.h"

#include <QDebug>

class StationsTracksModelResultEvent : public QEvent
{
public:
    static constexpr Type _Type = Type(CustomEvents::StationTracksListResult);
    inline StationsTracksModelResultEvent() : QEvent(_Type) {}

    QVector<StationTracksModel::TrackItem> items;
    int firstRow;
};

//Error messages

static constexpr char
    errorNameAlreadyUsedText[] = QT_TRANSLATE_NOOP("StationTracksModel",
                        "The track name <b>%1</b> is already used by another track of this station.");

static constexpr char
    errorLengthGreaterThanTrackText[] = QT_TRANSLATE_NOOP("StationTracksModel",
                        "Passenger and Freight train length must be less than or equal to track length.");

static constexpr char
    errorTrackInUseText[] = QT_TRANSLATE_NOOP("StationTracksModel",
                        "This track is still referenced.\n"
                        "Please remove all references before deleting it.");


StationTracksModel::StationTracksModel(sqlite3pp::database &db, QObject *parent) :
    IPagedItemModel(BatchSize, db, parent),
    m_stationId(0),
    cacheFirstRow(0),
    firstPendingRow(-BatchSize),
    editable(false)
{
    sortColumn = PosCol;
}

bool StationTracksModel::event(QEvent *e)
{
    if(e->type() == StationsTracksModelResultEvent::_Type)
    {
        StationsTracksModelResultEvent *ev = static_cast<StationsTracksModelResultEvent *>(e);
        ev->setAccepted(true);

        handleResult(ev->items, ev->firstRow);

        return true;
    }

    return QAbstractTableModel::event(e);
}

QVariant StationTracksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (section)
            {
            case NameCol:
                return tr("Name");
            case ColorCol:
                return tr("Color");
            case IsElectrifiedCol:
                return tr("Electrified");
            case IsThroughCol:
                return tr("Through");
            case TrackLengthCol:
                return tr("Length");
            case PassengerLegthCol:
                return tr("Passenger");
            case FreightLengthCol:
                return tr("Freight");
            case MaxAxesCol:
                return tr("Max Axes");
            }
            break;
        }
        case Qt::ToolTipRole:
        {
            switch (section)
            {
            case ColorCol:
                return tr("Leave white color to use default color.");
            case IsThroughCol:
                return tr("Through track: for non-stopping trains");
            case TrackLengthCol:
                return tr("Length of the track");
            case PassengerLegthCol:
                return tr("If the track is enabled for Passenger traffic,\n"
                          "set platform legth which is also maximum train length.");
            case FreightLengthCol:
                return tr("If the track is enabled for Freigth traffic,\n"
                          "set maximum train length.");
            case MaxAxesCol:
                return tr("Max train axes count.\n"
                          "Can be used instead of track length but is less precise.");
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


int StationTracksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : curItemCount;
}

int StationTracksModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant StationTracksModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<StationTracksModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const TrackItem& item = cache.at(row - cacheFirstRow);

    const QString fmt(QStringLiteral("%1 cm"));

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case NameCol:
            return item.name;
        case TrackLengthCol:
            return fmt.arg(item.trackLegth);
        case PassengerLegthCol:
            return item.platfLength ? fmt.arg(item.platfLength) : QVariant();
        case FreightLengthCol:
            return item.freightLength ? fmt.arg(item.freightLength) : QVariant();
        case MaxAxesCol:
            return item.maxAxesCount;
        }
        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case NameCol:
            return item.name;
        case TrackLengthCol:
            return item.trackLegth;
        case PassengerLegthCol:
            return item.platfLength;
        case FreightLengthCol:
            return item.freightLength;
        case MaxAxesCol:
            return item.maxAxesCount;
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        switch (idx.column())
        {
        case IsElectrifiedCol:
            return item.type.testFlag(utils::StationTrackType::Electrified) ? Qt::Checked : Qt::Unchecked;
        case IsThroughCol:
            return item.type.testFlag(utils::StationTrackType::Through) ? Qt::Checked : Qt::Unchecked;
        case PassengerLegthCol:
            return item.platfLength ? Qt::Checked : Qt::Unchecked;
        case FreightLengthCol:
            return item.freightLength ? Qt::Checked : Qt::Unchecked;
        }
        break;
    }
    case COLOR_ROLE:
    {
        if(idx.column() == ColorCol)
            return item.color;
        break;
    }
    }

    return QVariant();
}

bool StationTracksModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if(!editable)
        return false;

    const int row = idx.row();
    if(!idx.isValid() || row >= curItemCount || idx.column() >= NCols || row < cacheFirstRow || row >= cacheFirstRow + cache.size())
        return false; //Not fetched yet or invalid

    TrackItem &item = cache[row - cacheFirstRow];
    QModelIndex first = idx;
    QModelIndex last = idx;

    switch (role)
    {
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case NameCol:
        {
            QString str = value.toString().simplified();
            if(str.isEmpty() || !setName(item, str))
                return false;
            break;
        }
        default:
        {
            bool ok = false;
            int val = value.toInt(&ok);
            if(!ok || !setLength(item, val, Columns(idx.column())))
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
        case IsElectrifiedCol:
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            QFlags<utils::StationTrackType> type = item.type;
            type.setFlag(utils::StationTrackType::Electrified, cs == Qt::Checked);

            if(!setType(item, type))
                return false;
            break;
        }
        case IsThroughCol:
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            QFlags<utils::StationTrackType> type = item.type;
            type.setFlag(utils::StationTrackType::Through, cs == Qt::Checked);

            if(!setType(item, type))
                return false;
            break;
        }
        case PassengerLegthCol:
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            if(cs == Qt::Unchecked && item.platfLength != 0)
            {
                return setLength(item, 0, PassengerLegthCol);
            }
            else if(cs == Qt::Checked && item.platfLength == 0)
            {
                return setLength(item, item.trackLegth, PassengerLegthCol);
            }
            break;
        }
        case FreightLengthCol:
        {
            Qt::CheckState cs = value.value<Qt::CheckState>();
            if(cs == Qt::Unchecked && item.freightLength != 0)
            {
                return setLength(item, 0, FreightLengthCol);
            }
            else if(cs == Qt::Checked && item.freightLength == 0)
            {
                return setLength(item, item.trackLegth, FreightLengthCol);
            }
            break;
        }
        }
        break;
    }
    case COLOR_ROLE:
    {
        if(idx.column() != ColorCol)
            return false;
        bool ok = false;
        QRgb val = value.toInt(&ok);
        if(!ok || !setColor(item, val))
        break;
    }
    }

    emit dataChanged(first, last);

    return true;
}

Qt::ItemFlags StationTracksModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    if(idx.column() == IsElectrifiedCol || idx.column() == IsThroughCol)
        f.setFlag(Qt::ItemIsUserCheckable);
    else if(editable)
        f.setFlag(Qt::ItemIsEditable);

    return f;
}

void StationTracksModel::clearCache()
{
    cache.clear();
    cache.squeeze();
    cacheFirstRow = 0;
}

void StationTracksModel::refreshData()
{
    if(!mDb.db())
        return;

    emit itemsReady(-1, -1); //Notify we are about to refresh

    //TODO: consider filters
    query q(mDb, "SELECT COUNT(1) FROM station_tracks WHERE station_id=?");
    q.bind(1, m_stationId);
    q.step();
    const int count = q.getRows().get<int>(0);
    if(count != totalItemsCount)
    {
        beginResetModel();

        clearCache();
        totalItemsCount = count;
        emit totalItemsCountChanged(totalItemsCount);

        //Round up division
        const int rem = count % ItemsPerPage;
        pageCount = count / ItemsPerPage + (rem != 0);
        emit pageCountChanged(pageCount);

        if(curPage >= pageCount)
        {
            switchToPage(pageCount - 1);
        }

        curItemCount = totalItemsCount ? (curPage == pageCount - 1 && rem) ? rem : ItemsPerPage : 0;

        endResetModel();
    }
}

void StationTracksModel::setSortingColumn(int col)
{
    Q_UNUSED(col);
    return; //TODO: should enable sorting???
}

bool StationTracksModel::setStation(db_id stationId)
{
    m_stationId = stationId;
    clearCache();
    refreshData();
    return true;
}

bool StationTracksModel::addTrack(int pos, const QString &name, db_id *outTrackId)
{
    constexpr const int defaultTrackLength_cm = 150;

    if(name.isEmpty())
        return false;

    query q_getMaxPos(mDb, "SELECT MAX(pos) FROM station_tracks WHERE stationId=?");
    q_getMaxPos.bind(1, m_stationId);
    q_getMaxPos.step();
    int maxPos = 0;
    if(q_getMaxPos.getRows().column_type(0) != SQLITE_NULL)
        maxPos = q_getMaxPos.getRows().get<int>(0) + 1;
    q_getMaxPos.finish();

    command q_newTrack(mDb, "INSERT INTO station_gates"
                           "(id, station_id, pos, type, track_length_cm, platf_length_cm, freight_length_cm, max_axes, color_rgb, name)"
                           " VALUES (NULL, ?, ?, 0, ?, 0, 0, 2, NULL, ?)");
    q_newTrack.bind(1, m_stationId);
    q_newTrack.bind(2, maxPos);
    q_newTrack.bind(3, defaultTrackLength_cm);
    q_newTrack.bind(4, name);

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret = q_newTrack.execute();
    db_id trackId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_newTrack.reset();

    if((ret != SQLITE_OK && ret != SQLITE_DONE) || trackId == 0)
    {
        //Error
        if(outTrackId)
            *outTrackId = 0;

        if(ret == SQLITE_CONSTRAINT_UNIQUE)
        {
            emit modelError(tr(errorNameAlreadyUsedText).arg(name));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }
        return false;
    }

    if(outTrackId)
        *outTrackId = trackId;

    if(pos >= 0)
        moveTrack(trackId, pos);

    refreshData(); //Recalc row count
    switchToPage(0); //Reset to first page and so it is shown as first row

    return true;
}

bool StationTracksModel::removeTrack(db_id trackId)
{
    command q(mDb, "DELETE FROM station_tracks WHERE id=?");

    q.bind(1, trackId);
    int ret = q.execute();
    q.reset();

    if(ret != SQLITE_OK)
    {
        if(ret == SQLITE_CONSTRAINT_TRIGGER)
        {
            //TODO: show more information to the user, like where it's still referenced
            emit modelError(tr(errorTrackInUseText));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }

        return false;
    }

    refreshData(); //Recalc row count

    emit trackRemoved(trackId);

    return true;
}

bool StationTracksModel::moveTrackUpDown(db_id trackId, bool up, bool topOrBottom)
{
    query q(mDb);
    if(topOrBottom)
    {
        if(up)
        {
            q.prepare("SELECT MIN(pos) FROM station_tracks WHERE station_id=?");
        }else{
            q.prepare("SELECT MIN(pos) FROM station_tracks WHERE station_id=?");
        }
    }
    else
    {
        //Move by one
        if(up)
        {
            q.prepare("SELECT MAX(t1.pos) FROM station_tracks t1"
                      " JOIN station_tracks t ON t.id=?2"
                      " WHERE t1.station_id=?1 AND t.pos>t1.pos");
        }else{
            q.prepare("SELECT MIN(t1.pos) FROM station_tracks t1"
                      " JOIN station_tracks t ON t.id=?2"
                      " WHERE t1.station_id=?1 AND t.pos<t1.pos");
        }
        q.bind(2, trackId);
    }
    q.bind(1, m_stationId);
    q.step();
    int pos = q.getRows().get<int>(0);
    q.finish();

    moveTrack(trackId, pos);

    //Refresh model
    clearCache();

    return true;
}

void StationTracksModel::fetchRow(int row)
{
    if(firstPendingRow != -BatchSize)
        return; //Currently fetching another batch, wait for it to finish first

    if(row >= firstPendingRow && row < firstPendingRow + BatchSize)
        return; //Already fetching this batch

    if(row >= cacheFirstRow && row < cacheFirstRow + cache.size())
        return; //Already cached

    //TODO: abort fetching here

    const int remainder = row % BatchSize;
    firstPendingRow = row - remainder;
    qDebug() << "Requested:" << row << "From:" << firstPendingRow;

    QVariant val;
    int valRow = 0;


    //TODO: use a custom QRunnable
    //    QMetaObject::invokeMethod(this, "internalFetch", Qt::QueuedConnection,
    //                              Q_ARG(int, firstPendingRow), Q_ARG(int, sortCol),
    //                              Q_ARG(int, valRow), Q_ARG(QVariant, val));
    internalFetch(firstPendingRow, sortColumn, val.isNull() ? 0 : valRow, val);
}

void StationTracksModel::internalFetch(int first, int /*sortCol*/, int valRow, const QVariant &val)
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

    //const char *whereCol = nullptr;

    QByteArray sql = "SELECT id, type, track_length_cm, platf_length_cm, freight_length_cm,"
                     "max_axes, color_rgb, name FROM station_tracks ORDER BY pos";
//    switch (sortCol)
//    {
//    case LetterCol:
//    {
//        whereCol = "g.name"; //Order by 1 column, no where clause
//        break;
//    }
//    case SideCol:
//    {
//        whereCol = "g.side,g.name";
//        break;
//    }
//    }

    //    if(val.isValid())
    //    {
    //        sql += " WHERE ";
    //        sql += whereCol;
    //        if(reverse)
    //            sql += "<?3";
    //        else
    //            sql += ">?3";
    //    }
    sql += " WHERE station_id=?4";

//    sql += " ORDER BY ";
//    sql += whereCol;

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

    QVector<TrackItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    const QRgb whiteColor = qRgb(255, 255, 255);

    if(reverse)
    {
        int i = BatchSize - 1;

        for(; it != end; ++it)
        {
            auto r = *it;
            TrackItem &item = vec[i];
            item.trackId = r.get<db_id>(0);
            item.type = utils::StationTrackType(r.get<int>(1));
            item.trackLegth = r.get<int>(2);
            item.platfLength = r.get<int>(3);
            item.freightLength = r.get<int>(4);
            item.maxAxesCount = r.get<db_id>(5);
            if(r.column_type(6) == SQLITE_NULL)
                item.color = whiteColor;
            else
                item.color = QRgb(r.get<int>(6));
            item.name = r.get<QString>(7);
            i--;
        }
        if(i > -1)
            vec.remove(0, i + 1);
    }
    else
    {
        int i = 0;

        for(; it != end; ++it)
        {
            auto r = *it;
            TrackItem &item = vec[i];
            item.trackId = r.get<db_id>(0);
            item.type = utils::StationTrackType(r.get<int>(1));
            item.trackLegth = r.get<int>(2);
            item.platfLength = r.get<int>(3);
            item.freightLength = r.get<int>(4);
            item.maxAxesCount = r.get<db_id>(5);
            if(r.column_type(6) == SQLITE_NULL)
                item.color = whiteColor;
            else
                item.color = QRgb(r.get<int>(6));
            item.name = r.get<QString>(7);
            i++;
        }
        if(i < BatchSize)
            vec.remove(i, BatchSize - i);
    }


    StationsTracksModelResultEvent *ev = new StationsTracksModelResultEvent;
    ev->items = vec;
    ev->firstRow = first;

    qApp->postEvent(this, ev);
}

void StationTracksModel::handleResult(const QVector<StationTracksModel::TrackItem> &items, int firstRow)
{
    if(firstRow == cacheFirstRow + cache.size())
    {
        qDebug() << "RES: appending First:" << cacheFirstRow;
        cache.append(items);
        if(cache.size() > ItemsPerPage)
        {
            const int extra = cache.size() - ItemsPerPage; //Round up to BatchSize
            const int remainder = extra % BatchSize;
            const int n = remainder ? extra + BatchSize - remainder : extra;
            qDebug() << "RES: removing last" << n;
            cache.remove(0, n);
            cacheFirstRow += n;
        }
    }
    else
    {
        if(firstRow + items.size() == cacheFirstRow)
        {
            qDebug() << "RES: prepending First:" << cacheFirstRow;
            QVector<TrackItem> tmp = items;
            tmp.append(cache);
            cache = tmp;
            if(cache.size() > ItemsPerPage)
            {
                const int n = cache.size() - ItemsPerPage;
                cache.remove(ItemsPerPage, n);
                qDebug() << "RES: removing first" << n;
            }
        }
        else
        {
            qDebug() << "RES: replacing";
            cache = items;
        }
        cacheFirstRow = firstRow;
        qDebug() << "NEW First:" << cacheFirstRow;
    }

    firstPendingRow = -BatchSize;

    int lastRow = firstRow + items.count(); //Last row + 1 extra to re-trigger possible next batch
    if(lastRow >= curItemCount)
        lastRow = curItemCount -1; //Ok, there is no extra row so notify just our batch

    if(firstRow > 0)
        firstRow--; //Try notify also the row before because there might be another batch waiting so re-trigger it
    QModelIndex firstIdx = index(firstRow, 0);
    QModelIndex lastIdx = index(lastRow, NCols - 1);
    emit dataChanged(firstIdx, lastIdx);
    emit itemsReady(firstRow, lastRow);

    qDebug() << "TOTAL: From:" << cacheFirstRow << "To:" << cacheFirstRow + cache.size() - 1;
}

bool StationTracksModel::setName(StationTracksModel::TrackItem &item, const QString &name)
{
    //TODO: check non valid characters

    command q(mDb, "UPDATE station_tracks SET name=? WHERE id=?");
    q.bind(1, name);
    q.bind(2, item.trackId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        if(ret == SQLITE_CONSTRAINT_UNIQUE)
        {
            emit modelError(tr(errorNameAlreadyUsedText).arg(name));
        }
        else
        {
            emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        }
        return false;
    }

    item.name = name;
    emit trackNameChanged(item.trackId, item.name);

    if(sortColumn != PosCol)
    {
        //This row has now changed position so we need to invalidate cache
        //HACK: we emit dataChanged for this index (that doesn't exist anymore)
        //but the view will trigger fetching at same scroll position so it is enough
        cache.clear();
        cacheFirstRow = 0;
    }

    return true;
}

bool StationTracksModel::setType(StationTracksModel::TrackItem &item, QFlags<utils::StationTrackType> type)
{
    if(item.type == type)
        return false;

    //TODO: check through tracks must be default in at least 1 gate per side

    command q(mDb, "UPDATE station_tracks SET type=? WHERE id=?");
    q.bind(1, type);
    q.bind(2, item.trackId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        return false;
    }

    item.type = type;

    return true;
}

bool StationTracksModel::setLength(StationTracksModel::TrackItem &item, int val, StationTracksModel::Columns column)
{
    if(val < 0)
        return false;

    int *length = nullptr;
    const char *colName = nullptr;
    switch(column)
    {
    case TrackLengthCol:
    {
        length = &item.trackLegth;
        colName = "track_length_cm";
        break;
    }
    case PassengerLegthCol:
    {
        length = &item.platfLength;
        colName = "platf_length_cm";
        break;
    }
    case FreightLengthCol:
    {
        length = &item.freightLength;
        colName = "freight_length_cm";
        break;
    case MaxAxesCol:
    {
        length = &item.maxAxesCount;
        colName = "max_axes";
        break;
    }
    }
    default:
        return false;
    }

    if(val == *length)
        return false;

    if(val > item.trackLegth && column != TrackLengthCol && column != MaxAxesCol)
    {
        emit modelError(errorLengthGreaterThanTrackText);
        return false;
    }

    QByteArray sql = "UPDATE station_tracks SET ";
    sql.append(colName);
    sql.append("=? WHERE id=?");

    command q(mDb, sql.constData());
    q.bind(1, val);
    q.bind(2, item.trackId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        return false;
    }

    *length = val;

    return true;
}

bool StationTracksModel::setColor(StationTracksModel::TrackItem &item, QRgb color)
{
    if(item.color == color)
        return false;

    const QRgb whiteColor = qRgb(255, 255, 255);

    //TODO: check through tracks must be default in at least 1 gate per side

    command q(mDb, "UPDATE station_tracks SET color=? WHERE id=?");
    if(color == whiteColor)
        q.bind(1); //Bind NULL
    else
        q.bind(1, int(color));
    q.bind(2, item.trackId);
    int ret = q.step();
    if(ret != SQLITE_OK && ret != SQLITE_DONE)
    {
        emit modelError(tr("Error: %1").arg(mDb.error_msg()));
        return false;
    }

    item.color = color;

    return true;
}

void StationTracksModel::moveTrack(db_id trackId, int pos)
{
    command q_setPos(mDb, "UPDATE station_tracks SET pos=? WHERE id=?");

    query q(mDb, "SELECT id FROM station_tracks WHERE station_id=? AND pos=?");
    q.bind(m_stationId);
    q.bind(2, pos);
    if(q.step() == SQLITE_ROW)
    {
        if(q.getRows().get<db_id>(0) == trackId)
            return; //Already in position

        //Count tracks which need to be shifted
        q.prepare("SELECT COUNT(1) FROM station_tracks WHERE station_id=? AND pos>=?");
        q.bind(1, m_stationId);
        q.bind(2, pos);
        q.step();
        const int count = q.getRows().get<int>(0);
        int otherPos = pos + count;

        //Shift tracks by one (caution: there is UNIQUE constraint on pos)
        q.prepare("SELECT id FROM station_tracks WHERE station_id=? AND pos>=? ORDER BY pos DESC");
        q.bind(1, m_stationId);
        q.bind(2, pos);
        for(auto track : q)
        {
            db_id otherId = track.get<db_id>(0);
            q_setPos.bind(1, otherPos);
            q_setPos.bind(2, otherId);
            q_setPos.execute();
            q_setPos.reset();
            otherPos--;
        }
    }

    //Set position
    q_setPos.bind(1, pos);
    q_setPos.bind(2, trackId);
    q_setPos.execute();
    q_setPos.reset();
}
