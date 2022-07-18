#include "railwaysegmentconnectionsmodel.h"

#include <QBrush>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QDebug>

//Error messages
static constexpr char errorNegativeNumber[] =
    QT_TRANSLATE_NOOP("RailwaySegmentConnectionsModel",
                      "Cannot set negative number as track.");

static constexpr char errorOutOfBound[] =
    QT_TRANSLATE_NOOP("RailwaySegmentConnectionsModel",
                      "Track number out of bound.<br>"
                      "Gate has only <b>%1</b> tracks.");

static constexpr char errorAlreadyConnectedFrom[] =
    QT_TRANSLATE_NOOP("RailwaySegmentConnectionsModel",
                      "Track <b>%1</b> is already connected to track <b>%2</b>.");

static constexpr char errorAlreadyConnectedTo[] =
    QT_TRANSLATE_NOOP("RailwaySegmentConnectionsModel",
                      "Track <b>%1</b> is already connected from track <b>%2</b>.");


RailwaySegmentConnectionsModel::RailwaySegmentConnectionsModel(sqlite3pp::database &db, QObject *parent) :
    QAbstractTableModel(parent),
    mDb(db),
    m_segmentId(0),
    m_fromGateId(0),
    m_toGateId(0),
    m_fromGateTrackCount(0),
    m_toGateTrackCount(0),
    actualCount(0),
    incompleteRowAdded(-1),
    m_reversed(0),
    readOnly(false)
{

}

QVariant RailwaySegmentConnectionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (section)
            {
            case FromGateTrackCol:
                return tr("From");
            case ToGateTrackCol:
                return tr("To");
            }
            break;
        }
        }
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

int RailwaySegmentConnectionsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : items.size();
}

int RailwaySegmentConnectionsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant RailwaySegmentConnectionsModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.column() >= NCols)
        return QVariant();

    const RailwayTrack &item = items.at(idx.row());

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case FromGateTrackCol:
        {
            if(item.fromTrack == InvalidTrack)
                return tr("NULL");
            return item.fromTrack;
        }
        case ToGateTrackCol:
        {
            if(item.toTrack == InvalidTrack)
                return tr("NULL");
            return item.toTrack;
        }
        }
        break;
    }
    case Qt::EditRole:
    {
        switch (idx.column())
        {
        case FromGateTrackCol:
            return item.fromTrack;
        case ToGateTrackCol:
            return item.toTrack;
            break;
        }
        break;
    }
    case Qt::ToolTipRole:
    {
        switch (item.state)
        {
        case NoChange:
            return QVariant();
        case AddedButNotComplete:
            return tr("Incomplete");
        case ToAdd:
            return tr("To Add");
        case ToRemove:
            return tr("To Remove");
            break;
        case Edited:
            return tr("Edited");
        }

        break;
    }
    case Qt::BackgroundRole:
    {
        QColor color;

        switch (item.state)
        {
        case NoChange:
            return QVariant(); //No background
        case AddedButNotComplete:
            color = Qt::cyan;
            break;
        case ToAdd:
            color = Qt::green;
            break;
        case ToRemove:
            color = Qt::red;
            break;
        case Edited:
            color = Qt::yellow;
            break;
        }

        return QBrush(color.lighter());
    }
    }
    return QVariant();
}

bool RailwaySegmentConnectionsModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (readOnly || !idx.isValid() || idx.column() >= NCols || role != Qt::EditRole)
        return false;

    RailwayTrack& item = items[idx.row()];
    if(item.state == ToRemove)
        return false;

    bool ok = false;
    int trackNum = value.toInt(&ok);
    if(!ok || trackNum < 0)
    {
        emit modelError(tr(errorNegativeNumber));
        return false;
    }

    if(idx.column() == FromGateTrackCol)
    {
        if(trackNum == item.fromTrack)
            return false; //No change

        if(trackNum >= m_fromGateTrackCount)
        {
            //Out of bound
            emit modelError(tr(errorOutOfBound).arg(m_fromGateTrackCount));
            return false;
        }

        for(const RailwayTrack& other : qAsConst(items))
        {
            if(other.state != ToRemove && other.state != AddedButNotComplete && other.fromTrack == trackNum)
            {
                //Already connected
                emit modelError(tr(errorAlreadyConnectedFrom).arg(item.fromTrack).arg(item.toTrack));
                return false;
            }
        }

        item.fromTrack = trackNum;
    }
    else
    {
        if(trackNum == item.toTrack)
            return false; //No change

        if(trackNum >= m_toGateTrackCount)
        {
            //Out of bound
            emit modelError(tr(errorOutOfBound).arg(m_toGateTrackCount));
            return false;
        }

        for(const RailwayTrack& other : qAsConst(items))
        {
            if(other.state != ToRemove && other.state != AddedButNotComplete && other.toTrack == trackNum)
            {
                //Already connected
                emit modelError(tr(errorAlreadyConnectedTo).arg(item.toTrack).arg(item.fromTrack));
                return false;
            }
        }

        item.toTrack = trackNum;
    }

    if(item.state == NoChange)
    {
        item.state = Edited;
    }
    else if(item.state == AddedButNotComplete)
    {
        if(item.fromTrack > InvalidTrack && item.toTrack > InvalidTrack)
        {
            //Now it's complete so reset
            incompleteRowAdded = -1;
            if(item.connId)
                item.state = Edited; //Was recycled from ToRemove
            else
                item.state = ToAdd; //New to add
        }
    }

    emit dataChanged(idx, idx);
    return true;
}

Qt::ItemFlags RailwaySegmentConnectionsModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid() || idx.column() >= NCols)
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    //Prevent editing in removed rows
    const RailwayTrack &item = items.at(idx.row());
    if(!readOnly && item.state != ToRemove)
        f.setFlag(Qt::ItemIsEditable);

    return f;
}

void RailwaySegmentConnectionsModel::setSegment(db_id segmentId, db_id fromGateId, db_id toGateId, bool reversed)
{
    m_segmentId = segmentId;
    m_fromGateId = fromGateId;
    m_toGateId = toGateId;
    m_reversed = reversed;

    query q(mDb, "SELECT out_track_count FROM station_gates WHERE id=?");
    q.bind(1, m_fromGateId);
    q.step();
    m_fromGateTrackCount = q.getRows().get<int>(0);

    q.reset();
    q.bind(1, m_toGateId);
    q.step();
    m_toGateTrackCount = q.getRows().get<int>(0);
}

void RailwaySegmentConnectionsModel::resetData()
{
    beginResetModel();

    items.clear();
    actualCount = 0;
    incompleteRowAdded = -1;

    query q(mDb, "SELECT COUNT(1) FROM railway_connections WHERE seg_id=?");
    q.bind(1, m_segmentId);
    q.step();
    int count = q.getRows().get<int>(0);
    if(!count)
    {
        endResetModel();
        return;
    }

    q.prepare("SELECT id,in_track,out_track FROM railway_connections WHERE seg_id=? ORDER BY in_track");
    q.bind(1, m_segmentId);

    const int fromTrackCol = m_reversed ? 2 : 1;
    const int toTrackCol = m_reversed ? 1 : 2;

    for(auto conn : q)
    {
        RailwayTrack track;
        track.connId = conn.get<db_id>(0);
        track.fromTrack = conn.get<int>(fromTrackCol);
        track.toTrack = conn.get<int>(toTrackCol);

        if(track.fromTrack >= m_fromGateTrackCount || track.toTrack >= m_toGateTrackCount)
        {
            //Must remove this connection
            track.state = ToRemove;
        }
        else
        {
            track.state = NoChange;
            actualCount++;
        }

        items.append(track);
    }

    endResetModel();
}

void RailwaySegmentConnectionsModel::clear()
{
    beginResetModel();
    items.clear();
    actualCount = 0;
    incompleteRowAdded = -1;
    endResetModel();
}

void RailwaySegmentConnectionsModel::createDefaultConnections()
{
    if(readOnly || m_fromGateTrackCount == 0 || m_toGateTrackCount == 0)
        return;

    RailwayTrack track;
    track.connId = 0;
    track.state = ToAdd;
    track.fromTrack = 0;

    if(m_toGateTrackCount == 2 && m_fromGateTrackCount == 2)
    {
        //This is a double track line
        //Connect 0 -> 1 and 1 -> 0
        //Think it like elctonic pole ('+' -> '-' and '-' -> '+')

        //0 -> 1
        track.toTrack = 1;
        insertOrReplace(track);

        //1 -> 0
        track.fromTrack = 1;
        track.toTrack = 0;
        insertOrReplace(track);
    }
    else
    {
        //In every other case just connect first track to first track
        //0 -> 0
        track.toTrack = 0;
        insertOrReplace(track);
    }
}

void RailwaySegmentConnectionsModel::removeAtRow(int row)
{
    if(readOnly || row < 0 || row >= items.size())
        return;

    RailwayTrack &track = items[row];
    if(track.state == AddedButNotComplete)
        incompleteRowAdded = -1; //Reset

    if(track.state == ToAdd || (track.state == AddedButNotComplete && !track.connId))
    {
        //This track is not yet in the database so remove it
        beginRemoveRows(QModelIndex(), row, row);
        items.removeAt(row);
        actualCount--;
        endRemoveRows();
    }
    else
    {
        //This track is already in the database so mark it
        if(track.state != ToRemove)
            actualCount--;
        track.state = ToRemove;

        emit dataChanged(index(row, FromGateTrackCol),
                         index(row, ToGateTrackCol));
    }
}

void RailwaySegmentConnectionsModel::addNewConnection(int *outRow)
{
    if(readOnly)
    {
        if(outRow)
            *outRow = -1;
        return;
    }

    if(incompleteRowAdded >= 0)
    {
        //Already added and incomplete
        if(outRow)
            *outRow = incompleteRowAdded;
        return;
    }

    RailwayTrack track;
    track.fromTrack = InvalidTrack;
    track.toTrack = InvalidTrack;
    int row = insertOrReplace(track);
    if(row < 0)
        row = items.size() - 1; //Last (was appended)

    items[row].state = AddedButNotComplete;
    incompleteRowAdded = row;

    if(outRow)
        *outRow = row;
}

bool RailwaySegmentConnectionsModel::applyChanges(db_id overrideSegmentId)
{
    if(readOnly)
        return false;

    if(incompleteRowAdded >= 0)
    {
        RailwayTrack& item = items[incompleteRowAdded];
        if(item.fromTrack == InvalidTrack || item.toTrack == InvalidTrack)
        {
            //Item is still invalid
            if(item.connId)
            {
                //Item was recycled so remove it
                item.state = ToRemove;
            }
        }
    }

    //First remove all ToRemove
    command cmd(mDb, "DELETE FROM railway_connections WHERE id=?");
    for(const RailwayTrack& item : qAsConst(items))
    {
        if(item.state == ToRemove)
        {
            cmd.bind(1, item.connId);
            int ret = cmd.execute();
            if(ret != SQLITE_OK)
            {
                qWarning() << "Error removing track conn ID:" << item.connId << "Segment:" << m_segmentId
                           << "From:" << item.fromTrack << "To:" << item.toTrack
                           << mDb.error_msg();
            }
            cmd.reset();
        }
    }

    //Then edit all Edited
    cmd.prepare("UPDATE railway_connections SET in_track=?, out_track=? WHERE id=?");
    const int fromTrackCol = m_reversed ? 2 : 1;
    const int toTrackCol = m_reversed ? 1 : 2;
    for(const RailwayTrack& item : qAsConst(items))
    {
        if(item.state == Edited)
        {
            cmd.bind(fromTrackCol, item.fromTrack);
            cmd.bind(toTrackCol, item.toTrack);
            cmd.bind(3, item.connId);
            int ret = cmd.execute();
            if(ret != SQLITE_OK)
            {
                qWarning() << "Error editing track conn ID:" << item.connId << "Segment:" << m_segmentId
                           << "New From:" << item.fromTrack << "New To:" << item.toTrack
                           << mDb.error_msg();
            }
            cmd.reset();
        }
    }

    //Lock to retreive inserted ID
    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);

    //Finally add all ToAdd
    cmd.prepare("INSERT INTO railway_connections(id,seg_id,in_track,out_track) VALUES(NULL,?3,?1,?2)");
    for(RailwayTrack& item : items)
    {
        if(item.state == ToAdd)
        {
            cmd.bind(fromTrackCol, item.fromTrack);
            cmd.bind(toTrackCol, item.toTrack);
            cmd.bind(3, overrideSegmentId);
            int ret = cmd.execute();
            if(ret != SQLITE_OK)
            {
                qWarning() << "Error adding track conn to Segment:" << m_segmentId
                           << "From:" << item.fromTrack << "To:" << item.toTrack
                           << mDb.error_msg();
            }
            item.connId = mDb.last_insert_rowid();
            cmd.reset();
        }
    }

    //Unlock database
    sqlite3_mutex_leave(mutex);

    return true;
}

int RailwaySegmentConnectionsModel::insertOrReplace(const RailwaySegmentConnectionsModel::RailwayTrack &newTrack)
{
    //Check if track is already connected
    for(const RailwayTrack& other : qAsConst(items))
    {
        if(other.state == ToRemove || other.state == AddedButNotComplete)
            continue; //Ignore these items

        if(other.fromTrack == newTrack.fromTrack || other.toTrack == newTrack.toTrack)
        {
            //Track is already connected, cannot connect twice
            return TrackAlreadyConnected;
        }
    }

    actualCount++; //We will have one more row

    //Reuse removed rows if any
    for(int i = 0; i < items.size(); i++)
    {
        if(items.at(i).state == ToRemove)
        {
            RailwayTrack &track = items[i];

            //Copy track numbers
            track.fromTrack = newTrack.fromTrack;
            track.toTrack = newTrack.toTrack;

            //Set as edited so doesn't get removed
            track.state = Edited;

            emit dataChanged(index(i, FromGateTrackCol), index(i, ToGateTrackCol));

            return i;
        }
    }

    //No removed row found, insert new
    beginInsertRows(QModelIndex(), items.size(), items.size());
    items.append(newTrack);
    items.last().connId = 0;
    items.last().state = ToAdd;
    endInsertRows();
    return NewTrackAdded;
}
