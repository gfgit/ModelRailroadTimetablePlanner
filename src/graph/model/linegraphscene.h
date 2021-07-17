#ifndef LINEGRAPHSCENE_H
#define LINEGRAPHSCENE_H

#include <QObject>
#include <QVector>
#include <QHash>

#include "utils/types.h"

#include "stationgraphobject.h"

namespace sqlite3pp {
class database;
}

enum class LineGraphType
{
    NoGraph = 0,
    SingleStation,
    RailwaySegment,
    RailwayLine
};

class LineGraphScene : public QObject
{
    Q_OBJECT
public:
    LineGraphScene(sqlite3pp::database &db, QObject *parent = nullptr);

    bool loadGraph(db_id objectId, LineGraphType type, bool force = false);

    inline LineGraphType getGraphType() const
    {
        return graphType;
    }

    inline db_id getGraphObjectId() const
    {
        return graphObjectId;
    }

    inline QString getGraphObjectName() const
    {
        return graphObjectName;
    }

signals:
    void graphChanged(int type, db_id objectId);
    void redrawGraph();

public slots:
    void reload();

private:
    bool loadStation(StationGraphObject &st);
    bool loadFullLine(db_id lineId);

private:
    friend class LineGraphViewport;
    friend class StationLabelsHeader;
    friend class LineGraphManager;

    sqlite3pp::database& mDb;

    /* Can be station/segment/line ID depending on graph type */
    db_id graphObjectId;
    LineGraphType graphType;

    QString graphObjectName;

    typedef struct
    {
        db_id stationId;
        double xPos;
    } StationPosEntry;

    QVector<StationPosEntry> stationPositions;
    QHash<db_id, StationGraphObject> stations;
};

#endif // LINEGRAPHSCENE_H
