#ifndef TRAINGRAPHICS_H
#define TRAINGRAPHICS_H

#include <QtGlobal>

#include <QVector>
#include <QHash>

#include "utils/types.h"

class QGraphicsScene;
class QGraphicsPathItem;
class QGraphicsSimpleTextItem;

class SegmentGraph
{
public:
    SegmentGraph(db_id seg = 0, db_id line = 0);
    void cleanup();

    db_id segId;
    db_id lineId;
    QGraphicsPathItem *item;
    QVector<QGraphicsSimpleTextItem *> texts;
};

//FIXME: remove, class is deprecated
class TrainGraphics
{
public:
    TrainGraphics(db_id id, JobCategory c = JobCategory::FREIGHT);

    void setCategory(JobCategory cat);
    void updateColor();


    db_id m_jobId;
    JobCategory category;

    QHash<db_id, SegmentGraph> segments;

    void setId(db_id newId);

    void recalcPath();
    void drawSegment(db_id segId, db_id lineId);
    void createSegment(db_id segId);
    void clear();
};

#endif // TRAINGRAPHICS_H
