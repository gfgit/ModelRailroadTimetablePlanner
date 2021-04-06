#include "stationgatesmatchmodel.h"

#include "station_name_utils.h"

#include <QBrush>

using namespace sqlite3pp;

StationGatesMatchModel::StationGatesMatchModel(sqlite3pp::database &db, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    q_getMatches(mDb, "SELECT id,type,name,side"
                      " FROM station_gates WHERE station_id=?2 AND name LIKE ?1"
                      " ORDER BY side,name"),
    m_stationId(0),
    m_excludeSegmentId(0),
    m_markConnectedGates(false)
{
}

QVariant StationGatesMatchModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= size)
        return QVariant();

    const bool emptyRow = hasEmptyRow &&
                          (idx.row() == ItemCount || (size < ItemCount + 2 && idx.row() == size - 1));
    const bool ellipsesRow = idx.row() == (ItemCount - 1 - (hasEmptyRow ? 0 : 1));

    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(emptyRow)
        {
            return ISqlFKMatchModel::tr("Empty");
        }
        else if(ellipsesRow)
        {
            return ellipsesString;
        }

        return items[idx.row()].gateLetter;
    }
    case Qt::ToolTipRole:
    {
        if(!emptyRow && !ellipsesRow)
        {
            QString tip = tr("Gate <b>%1</b> is %2")
                              .arg(items[idx.row()].gateLetter,
                                   utils::StationUtils::name(items[idx.row()].side));
            if(m_markConnectedGates)
            {
                QString state;
                db_id segId = items[idx.row()].segmentId;
                if(segId)
                {
                    state = tr("Segment: <b>%1</b>").arg(items[idx.row()].segmentName);
                    if(segId == m_excludeSegmentId)
                        state.append(tr("<br>Current"));
                }
                else
                {
                    state = tr("Not connected");
                }

                //New line, then append
                tip.append("<br>");
                tip.append(state);
            }
            return tip;
        }
        break;
    }
    case Qt::FontRole:
    {
        if(emptyRow)
        {
            return boldFont();
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        if(!emptyRow && !ellipsesRow)
            return Qt::AlignRight + Qt::AlignVCenter;
        break;
    }
    case Qt::BackgroundRole:
    {
        if(!emptyRow && !ellipsesRow && m_markConnectedGates)
        {
            db_id segId = items[idx.row()].segmentId;
            if(segId && segId != m_excludeSegmentId)
            {
                //Cyan if gate is connected to a segment
                return QBrush(Qt::cyan);
            }
        }
        break;
    }
    case Qt::DecorationRole:
    {
        if(!emptyRow && !ellipsesRow)
        {
            QColor color;
            if(items[idx.row()].type.testFlag(utils::GateType::Bidirectional))
                break; //Default color

            if(items[idx.row()].type.testFlag(utils::GateType::Entrance))
                color = Qt::green; //Entrance only
            else if(items[idx.row()].type.testFlag(utils::GateType::Exit))
                color = Qt::red; //Exit only
            return color;
        }
        break;
    }
    }

    return QVariant();
}

void StationGatesMatchModel::autoSuggest(const QString &text)
{
    mQuery.clear();
    if(!text.isEmpty())
    {
        mQuery.clear();
        mQuery.reserve(text.size() + 2);
        mQuery.append('%');
        mQuery.append(text.toUtf8());
        mQuery.append('%');
    }

    refreshData();
}

void StationGatesMatchModel::refreshData()
{
    if(!mDb.db())
        return;

    beginResetModel();

    char emptyQuery = '%';

    if(mQuery.isEmpty())
        sqlite3_bind_text(q_getMatches.stmt(), 1, &emptyQuery, 1, SQLITE_STATIC);
    else
        sqlite3_bind_text(q_getMatches.stmt(), 1, mQuery, mQuery.size(), SQLITE_STATIC);

    q_getMatches.bind(2, m_stationId);

    auto end = q_getMatches.end();
    auto it = q_getMatches.begin();
    int i = 0;
    for(; i < ItemCount && it != end; i++)
    {
        auto track = *it;
        items[i].gateId = track.get<db_id>(0);
        items[i].type = utils::GateType(track.get<int>(1));
        items[i].gateLetter = sqlite3_column_text(q_getMatches.stmt(), 2)[0];
        items[i].side = utils::Side(track.get<int>(3));

        if(m_markConnectedGates)
        {
            items[i].segmentId = track.get<db_id>(4);
            items[i].segmentName = track.get<QString>(5);
        }else{
            items[i].segmentId = 0;
            items[i].segmentName.clear();
        }

        ++it;
    }

    size = i;
    if(hasEmptyRow)
        size++; //Items + Empty

    if(it != end)
    {
        //There would be still rows, show Ellipses
        size++; //Items + Empty + Ellispses
    }

    q_getMatches.reset();
    endResetModel();

    emit resultsReady(false);
}

QString StationGatesMatchModel::getName(db_id id) const
{
    if(!mDb.db())
        return QString();

    query q(mDb, "SELECT name FROM station_gates WHERE id=?");
    q.bind(1, id);
    if(q.step() == SQLITE_ROW)
        return q.getRows().get<QString>(0);
    return QString();
}

db_id StationGatesMatchModel::getIdAtRow(int row) const
{
    return items[row].gateId;
}

QString StationGatesMatchModel::getNameAtRow(int row) const
{
    return items[row].gateLetter;
}

void StationGatesMatchModel::setFilter(db_id stationId, bool markConnectedGates, db_id excludeSegmentId)
{
    m_stationId = stationId;
    m_markConnectedGates = markConnectedGates;
    m_excludeSegmentId = m_markConnectedGates ? excludeSegmentId : 0;

    QByteArray sql = "SELECT g.id,g.type,g.name,g.side";
    if(m_markConnectedGates)
    {
        sql += ",s.id,s.name";
    }
    sql += " FROM station_gates g";
    if(m_markConnectedGates)
    {
        sql += " LEFT JOIN railway_segments s ON s.in_gate_id=g.id OR s.out_gate_id=g.id";
    }

    sql += " WHERE g.station_id=?2 AND g.name LIKE ?1";

    sql += " ORDER BY g.side,g.name";

    q_getMatches.prepare(sql);

    refreshData();
}

StationGatesMatchFactory::StationGatesMatchFactory(database &db, QObject *parent) :
    IMatchModelFactory(parent),
    m_stationId(0),
    m_excludeSegmentId(0),
    markConnectedGates(false),
    mDb(db)
{

}

ISqlFKMatchModel *StationGatesMatchFactory::createModel()
{
    StationGatesMatchModel *m = new StationGatesMatchModel(mDb);
    m->setFilter(m_stationId, markConnectedGates, m_excludeSegmentId);
    return m;
}
