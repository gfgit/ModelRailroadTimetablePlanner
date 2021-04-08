#include "railwaysegmentmatchmodel.h"

#include <QBrush>

using namespace sqlite3pp;

RailwaySegmentMatchModel::RailwaySegmentMatchModel(sqlite3pp::database &db, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    q_getMatches(mDb),
    m_fromStationId(0),
    m_toStationId(0),
    m_excludeSegmentId(0)
{

}

QVariant RailwaySegmentMatchModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= size)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(isEmptyRow(idx.row()))
        {
            return ISqlFKMatchModel::tr("Empty");
        }
        else if(isEllipsesRow(idx.row()))
        {
            return ellipsesString;
        }

        return items[idx.row()].segmentName;
    }
    case Qt::DecorationRole:
    {
        //Draw a small blue square for electified segments
        if(!isEmptyRow(idx.row()) && isEllipsesRow(idx.row()) &&
            items[idx.row()].type.testFlag(utils::RailwaySegmentType::Electrified))
        {
            return QColor(Qt::blue);
        }
        break;
    }
    case Qt::ToolTipRole:
    {
        if(!isEmptyRow(idx.row()) && !isEllipsesRow(idx.row()))
        {
            QStringList tips;

            //Electrification
            if(items[idx.row()].type.testFlag(utils::RailwaySegmentType::Electrified))
                tips.append(tr("Electrified"));
            else
                tips.append(tr("Non electrified"));

            //Direction
            if(items[idx.row()].reversed)
                tips.append(tr("Segment is reversed."));

            return tr("Segment <b>%1</b><br>%2").arg(items[idx.row()].segmentName, tips.join("<br>"));
        }
        break;
    }
    case Qt::BackgroundRole:
    {
        if(!isEmptyRow(idx.row()) && !isEllipsesRow(idx.row()) && items[idx.row()].reversed)
        {
            //Light cyan background for reversed segments
            return QBrush(qRgb(158, 226, 255)); //#9EE2FF
        }
        break;
    }
    case Qt::FontRole:
    {
        if(isEmptyRow(idx.row()))
        {
            return boldFont();
        }
        break;
    }
    }

    return QVariant();
}

void RailwaySegmentMatchModel::autoSuggest(const QString &text)
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

void RailwaySegmentMatchModel::refreshData()
{
    if(!mDb.db())
        return;

    beginResetModel();

    char emptyQuery = '%';

    if(mQuery.isEmpty())
        sqlite3_bind_text(q_getMatches.stmt(), 1, &emptyQuery, 1, SQLITE_STATIC);
    else
        sqlite3_bind_text(q_getMatches.stmt(), 1, mQuery, mQuery.size(), SQLITE_STATIC);

    if(m_fromStationId)
    {
        q_getMatches.bind(2, m_fromStationId);
        if(m_toStationId)
            q_getMatches.bind(3, m_toStationId);
    }
    if(m_excludeSegmentId)
    {
        q_getMatches.bind(4, m_excludeSegmentId);
    }

    auto end = q_getMatches.end();
    auto it = q_getMatches.begin();
    int i = 0;
    for(; i < MaxMatchItems && it != end; i++)
    {
        auto seg = *it;
        items[i].segmentId = seg.get<db_id>(0);
        items[i].segmentName = seg.get<QString>(1);
        items[i].type = utils::RailwaySegmentType(seg.get<int>(2));

        db_id inStationId = seg.get<db_id>(3);
        db_id outStationId = seg.get<db_id>(4);

        if(!m_fromStationId || inStationId == m_fromStationId)
        {
            items[i].toStationId = outStationId;
            items[i].reversed = false;
        }else{
            items[i].toStationId = inStationId;
            items[i].reversed = true;
        }

        ++it;
    }

    size = i + 1; //Items + Empty

    if(it != end)
    {
        //There would be still rows, show Ellipses
        size++; //Items + Empty + Ellispses
    }

    q_getMatches.reset();
    endResetModel();

    emit resultsReady(false);
}

QString RailwaySegmentMatchModel::getName(db_id id) const
{
    if(!mDb.db())
        return QString();

    query q(mDb, "SELECT name FROM railway_segments WHERE id=?");
    q.bind(1, id);
    if(q.step() == SQLITE_ROW)
        return q.getRows().get<QString>(0);
    return QString();
}

db_id RailwaySegmentMatchModel::getIdAtRow(int row) const
{
    return items[row].segmentId;
}

QString RailwaySegmentMatchModel::getNameAtRow(int row) const
{
    return items[row].segmentName;
}

void RailwaySegmentMatchModel::setFilter(db_id fromStationId, db_id toStationId, db_id excludeSegmentId)
{
    m_fromStationId = fromStationId;
    m_toStationId = toStationId;
    if(!m_fromStationId)
        m_toStationId = 0; //NOTE: don't filter by destination only
    m_excludeSegmentId = excludeSegmentId;

    QByteArray sql = "SELECT seg.id, seg.name, seg.type, g1.station_id, g2.station_id"
                     " FROM railway_segments seg"
                     " JOIN station_gates g1 ON g1.id=seg.in_gate_id"
                     " JOIN station_gates g2 ON g2.id=seg.out_gate_id"
                     " WHERE ";

    if(m_fromStationId)
    {
        sql += "((g1.station_id=?2";
        if(m_toStationId)
            sql += " AND g2.station_id=?3";
        sql += ") OR (g2.station_id=?2";
        if(m_toStationId)
            sql += " AND g1.station_id=?3";
        sql += ")) AND ";
    }
    if(m_excludeSegmentId)
    {
        sql += "seg.id<>?4 AND ";
    }

    sql.append("(seg.name LIKE ?1) LIMIT " QT_STRINGIFY(MaxMatchItems + 1));

    q_getMatches.prepare(sql.constData());
}

bool RailwaySegmentMatchModel::isReversed(db_id segId) const
{
    for(const SegmentItem& item : items)
    {
        if(item.segmentId == segId)
            return item.reversed;
    }
    return false;
}
