#ifndef SHIFTOBJ_H
#define SHIFTOBJ_H

#include <QString>
#include <QHash>
#include <QVector>

#include "utils/types.h"

class QGraphicsScene;
class QGraphicsSimpleTextItem;
class QGraphicsLineItem;

class ShiftGraphObj
{
public:
    ShiftGraphObj(db_id id, QGraphicsScene *scene);
    ~ShiftGraphObj();

    QString getName() const;
    void setName(const QString &value, double horizOffset);

    void clearJobs();

    void removeJob(db_id jobId);

    void centerName(double horizOffset);

    db_id shiftId;

    QGraphicsSimpleTextItem *text;
    QGraphicsLineItem *line;

    typedef struct JobGraph_
    {
        QGraphicsLineItem *line            = nullptr;
        QGraphicsSimpleTextItem *name      = nullptr;
        QGraphicsSimpleTextItem *st1_label = nullptr;
        QGraphicsSimpleTextItem *st2_label = nullptr;
        int pos;
    } JobGraph;

    typedef QHash<db_id, JobGraph> Jobs;
    Jobs jobs;
    QVector<Jobs::iterator> vec;

    int pos;
};

#endif // SHIFTOBJ_H
