#ifndef LINEOBJ_H
#define LINEOBJ_H

#include <QString>

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QHash>

#include <QRgb>

class GraphicsScene;
class QGraphicsLineItem;
class QGraphicsSimpleTextItem;

class StationObj
{
public:
    StationObj();

    db_id stId = 0;

    qint8 platfs = 1;
    qint8 depots = 0;

    QRgb platfRgb;

    class JobStop
    {
    public:
        bool operator==(const JobStop& s) { return this->stopId == s.stopId; }

        db_id stopId;
        QGraphicsLineItem *line;
        QGraphicsSimpleTextItem *text;
    };

    class Graph
    {
    public:
        qreal xPos = 0.0;
        db_id lineId = 0;
        db_id stationId = 0;
        QVector<QGraphicsLineItem *> platforms;

        QMultiHash<db_id, JobStop> stops;
    };

    QHash<db_id, Graph*> lineGraphs;

    void removeJob(db_id jobId);
    void addJobStop(db_id stopId, db_id jobId, db_id lineId, const QString &label, qreal y1, qreal y2, int platf);

    void setPlatfColor(QRgb rgb);
};

class LineObj
{
public:
    LineObj();

    db_id lineId;
    GraphicsScene *scene;

    QVector<StationObj::Graph*> stations;

    int refCount;
};

#endif // LINEOBJ_H
