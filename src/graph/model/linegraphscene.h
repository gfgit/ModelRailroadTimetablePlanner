#ifndef LINEGRAPHSCENE_H
#define LINEGRAPHSCENE_H

#include <QObject>
#include <QVector>
#include <QHash>

#include <QPointF>
#include <QSize>

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

    bool reloadJobs();

    JobEntry getJobAt(const QPointF& pos, const double tolerance);

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

    inline QSize getContentSize() const
    {
        return contentSize;
    }

signals:
    void graphChanged(int type, db_id objectId);
    void redrawGraph();

public slots:
    void reload();

private:

    typedef struct
    {
        db_id stationId;
        db_id segmentId;
        double xPos;
    } StationPosEntry;

private:
    void recalcContentSize();
    bool loadStation(StationGraphObject &st);
    bool loadFullLine(db_id lineId);

    bool loadStationJobStops(StationGraphObject &st);
    bool loadSegmentJobs(StationPosEntry &stPos, const StationGraphObject &fromStm, const StationGraphObject &toSt);

private:
    friend class BackgroundHelper;
    friend class LineGraphManager;

    sqlite3pp::database& mDb;

    /* Can be station/segment/line ID depending on graph type */
    db_id graphObjectId;
    LineGraphType graphType;

    QString graphObjectName;

    QVector<StationPosEntry> stationPositions;
    QHash<db_id, StationGraphObject> stations;

    QSize contentSize;
};

#endif // LINEGRAPHSCENE_H
