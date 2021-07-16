#ifndef LINEGRAPHSCENE_H
#define LINEGRAPHSCENE_H

#include <QObject>
#include <QVector>
<<<<<<< HEAD
#include <QHash>
=======
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e

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

public:
    bool loadGraph(db_id objectId, LineGraphType type);

private:
<<<<<<< HEAD
    bool loadStation(StationGraphObject &st);
=======
    bool loadStation(db_id stationId, StationGraphObject &st);
>>>>>>> 9981b07f0d414c83a475b7b4a6a36337bd99d54e

private:
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
