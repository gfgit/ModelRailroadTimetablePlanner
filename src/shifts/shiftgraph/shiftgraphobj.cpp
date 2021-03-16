#include "shiftgraphobj.h"

#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsLineItem>

ShiftGraphObj::ShiftGraphObj(db_id id, QGraphicsScene *scene) :
    shiftId(id),
    pos(0)
{
    text = scene->addSimpleText(QString());
    text->setBrush(Qt::red);
    QFont f;
    //f.setBold(true);
    f.setPointSize(12);
    text->setFont(f);
    line = scene->addLine(QLineF());
}

ShiftGraphObj::~ShiftGraphObj()
{
    delete text;
    delete line;

    clearJobs();
}

QString ShiftGraphObj::getName() const
{
    return text->text();
}

void ShiftGraphObj::setName(const QString &value, double horizOffset)
{
    text->setText(value);
    centerName(horizOffset);
}

inline void cleanUp(ShiftGraphObj::JobGraph& g)
{
    delete g.line;
    delete g.name;

    if(g.st1_label)
        delete g.st1_label; //Can be NULL if HideSameStations is set (default)

    delete g.st2_label;
}

void ShiftGraphObj::clearJobs()
{
    for(JobGraph& g : jobs)
    {
        cleanUp(g);
    }
    jobs.clear();
    vec.clear();
}

void ShiftGraphObj::removeJob(db_id jobId)
{
    auto it = jobs.find(jobId);
    if(it == jobs.end())
        return;
    JobGraph& g = it.value();

    cleanUp(g);

    int jobPos = g.pos;

    vec.removeAt(jobPos);
    for(int i = jobPos; i < vec.size(); i++)
    {
        vec[i].value().pos--;
    }
}

void ShiftGraphObj::centerName(double horizOffset)
{
    double y = text->y();
    QRectF r = text->boundingRect();
    r.moveCenter(QPointF(horizOffset / 2, r.center().y()));
    text->setPos(r.left(), y);
}
