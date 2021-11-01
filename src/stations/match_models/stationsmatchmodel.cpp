#include "stationsmatchmodel.h"

StationsMatchModel::StationsMatchModel(database &db, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    q_getMatches(mDb),
    mFromStationId(0),
    mExceptStId(0)
{

}

QVariant StationsMatchModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= size)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
        switch (idx.column())
        {
        case StationCol:
        {
            if(isEmptyRow(idx.row()))
            {
                return ISqlFKMatchModel::tr("Empty");
            }
            else if(isEllipsesRow(idx.row()))
            {
                return ellipsesString;
            }

            return items[idx.row()].stationName;
        }
        case SegmentCol:
        {
            if(isEmptyRow(idx.row()) || isEllipsesRow(idx.row()))
            {
                return QVariant(); //Emprty cell
            }

            return items[idx.row()].segmentName;
        }
        }

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

int StationsMatchModel::columnCount(const QModelIndex &p) const
{
    if(!p.isValid())
        return 0;
    return mFromStationId ? 2 : 1;
}

void StationsMatchModel::autoSuggest(const QString &text)
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

void StationsMatchModel::refreshData()
{
    if(!mDb.db())
        return;

    beginResetModel();

    char emptyQuery = '%';

    if(mFromStationId)
        q_getMatches.bind(1, mFromStationId);
    if(mExceptStId)
        q_getMatches.bind(2, mExceptStId);

    if(mQuery.isEmpty())
        sqlite3_bind_text(q_getMatches.stmt(), 3, &emptyQuery, 1, SQLITE_STATIC);
    else
        sqlite3_bind_text(q_getMatches.stmt(), 3, mQuery, mQuery.size(), SQLITE_STATIC);

    auto end = q_getMatches.end();
    auto it = q_getMatches.begin();
    int i = 0;
    for(; i < MaxMatchItems && it != end; i++)
    {
        auto r = *it;
        items[i].stationId = r.get<db_id>(0);
        items[i].stationName = r.get<QString>(1);
        if(mFromStationId)
        {
            items[i].segmentId = r.get<db_id>(2);
            items[i].segmentName = r.get<QString>(3);
        }
        else
        {
            items[i].segmentId = 0;
            items[i].segmentName.clear();
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

QString StationsMatchModel::getName(db_id id) const
{
    if(!mDb.db())
        return QString();

    query q(mDb, "SELECT name FROM stations WHERE id=?");
    q.bind(1, id);
    if(q.step() == SQLITE_ROW)
        return q.getRows().get<QString>(0);
    return QString();
}

db_id StationsMatchModel::getIdAtRow(int row) const
{
    return items[row].stationId;
}

QString StationsMatchModel::getNameAtRow(int row) const
{
    return items[row].stationName;
}

db_id StationsMatchModel::getSegmentAtRow(int row) const
{
    return items[row].segmentId;
}

void StationsMatchModel::setFilter(db_id connectedToStationId, db_id exceptStId)
{
    mFromStationId = connectedToStationId;
    mExceptStId = exceptStId;

    QByteArray sql;
    if(mFromStationId)
    {
        sql = "SELECT s.id, s.name, seg.id, seg.name"
              " FROM railway_segments seg"
              " JOIN station_gates g1 ON g1.id=seg.in_gate_id"
              " JOIN station_gates g2 ON g2.id=seg.out_gate_id"
              " JOIN stations s ON CASE WHEN g1.station_id<>?1 THEN s.id=g1.station_id ELSE s.id=g2.station_id END"
              " JOIN stations s2 ON s2.id=g2.station_id"
              " WHERE ";
        if(mExceptStId)
        {
            sql += "((g1.station_id=?1 AND g2.station_id<>?2) OR"
                   "(g2.station_id=?1 AND g1.station_id<>?2))";
        }
        else
        {
            sql += "(g1.station_id=?1 OR g2.station_id=?1)";
        }

        sql += "AND s.name LIKE ?3 OR s.short_name LIKE ?3) LIMIT " QT_STRINGIFY(MaxMatchItems + 1);
    }
    else
    {
        sql = "SELECT stations.id,stations.name FROM stations WHERE ";
        if(mExceptStId)
            sql.append("stations.id<>?2 AND ");
        sql.append("(stations.name LIKE ?3 OR stations.short_name LIKE ?3) LIMIT " QT_STRINGIFY(MaxMatchItems + 1));
    }

    q_getMatches.prepare(sql.constData());
}

StationMatchFactory::StationMatchFactory(database &db, QObject *parent) :
    IMatchModelFactory(parent),
    mFromStationId(0),
    mExceptStId(0),
    mDb(db)
{

}

ISqlFKMatchModel *StationMatchFactory::createModel()
{
    StationsMatchModel *m = new StationsMatchModel(mDb);
    m->setFilter(mFromStationId, mExceptStId);
    return m;
}
