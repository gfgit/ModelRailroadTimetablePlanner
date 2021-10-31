#include "traingraphics.h"
#include "app/session.h"
#include "utils/jobcategorystrings.h"

#include "app/scopedebug.h"

#include <QGraphicsPathItem>
#include <QPen>
#include "utils/model_roles.h"

#include <QTime>

//#include "jobs/jobstorage.h"

#include "sqlite3pp/sqlite3pp.h"
using namespace sqlite3pp;

constexpr qreal MSEC_PER_HOUR = 1000 * 60 * 60;

static qreal timeToNum(const QTime &t)
{
    qreal ret = t.msecsSinceStartOfDay() / MSEC_PER_HOUR;
    qDebug() << t << ret;
    return ret;
}

TrainGraphics::TrainGraphics(db_id id, JobCategory c) :
    m_jobId(id),
    category(c)
{

}

void TrainGraphics::clear()
{
    for(SegmentGraph &seg : segments)
        seg.cleanup();
    segments.clear();
}

void TrainGraphics::setCategory(JobCategory cat)
{
    if(category == cat)
        return;
    category = cat;

    updateColor();
}

void TrainGraphics::updateColor()
{
    const QColor col = Session->colorForCat(category);
    QPen pen(col, Session->jobLineWidth);
    QBrush brush(col);
    for(SegmentGraph& s : segments)
    {
        s.item->setPen(pen);
        for(auto t : qAsConst(s.texts))
            t->setBrush(brush);
    }
}

//NOTE: redraw job after setting a new id
void TrainGraphics::setId(db_id newId)
{
    //Reset stop labels before jobId is changed otherwise we cannot delete them because id doesn't match anymore

    m_jobId = newId;
}


#define MID(a, b) ((a+b)/2)

/* New methods */


void TrainGraphics::recalcPath()
{
//    DEBUG_ENTRY;

//    clear();

//    LineStorage *lines = Session->mLineStorage;

//    query q_selectSegments(Session->m_Db, "SELECT id, lineId FROM jobsegments WHERE jobId=?");
//    q_selectSegments.bind(1, m_jobId);
//    for(query::iterator it = q_selectSegments.begin(); it != q_selectSegments.end(); ++it)
//    {
//        const query::rows &segment = *it;

//        db_id segId = segment.get<db_id>(0);
//        db_id lineId = segment.get<db_id>(1);
//        if(lines->sceneForLine(lineId))
//        {
//            createSegment(segId);
//            drawSegment(segId, lineId);
//        }
//    }
}

void TrainGraphics::createSegment(db_id segId)
{
    SegmentGraph s(segId);
    auto i = new QGraphicsPathItem();
    i->setData(JOB_ID_ROLE, m_jobId);
    i->setData(SEGMENT_ROLE, segId);
    i->setFlag(QGraphicsItem::ItemIsSelectable);
    s.item = i;

    segments.insert(segId, s);
}

void TrainGraphics::drawSegment(db_id segId, db_id lineId)
{
    DEBUG_ENTRY;

    QString name = JobCategoryName::jobName(m_jobId, category);

    auto seg = segments.find(segId);
    if(seg == segments.end())
        return;

    query q_selectSegStops(Session->m_Db, "SELECT id,"
                                    "stationId,"
                                    "arrival,"
                                    "departure,"
                                    "platform,"
                                    "transit,"
                                    "otherSegment"
                                    " FROM stops"
                                    " WHERE segmentId=?1 OR otherSegment=?1"
                                    " ORDER BY arrival ASC");

    SegmentGraph& s = seg.value();
    s.lineId = lineId;
    s.item->setData(LINE_ID, lineId);

    QFont labelFont;
    labelFont.setPointSizeF(AppSettings.getJobLabelFontSize());
    const QColor col = Session->colorForCat(category);
    QPen pen(col, Session->jobLineWidth);
    QBrush brush(col);
    s.item->setPen(pen);

    int vertOffset = Session->vertOffset;
    int hourOffset = Session->hourOffset;

    QVector<QPointF> vec;
    QVector<bool> angles;

    q_selectSegStops.bind(1, segId);
    q_selectSegStops.bind(2, segId);
    query::iterator it = q_selectSegStops.begin();
    if(it == q_selectSegStops.end())
    {
        qWarning() << "Error: no stops registered for job" << m_jobId;
        return;
    }
    else
    {
        qInfo() << "Calculatin path of job" << m_jobId;
    }

    db_id stopId = (*it).get<db_id>(0);
    db_id stId = (*it).get<db_id>(1);
    QTime arrival = (*it).get<QTime>(2);
    QTime departure = (*it).get<QTime>(3);
    int platf = (*it).get<int>(4);

    qreal hour1 = timeToNum(arrival);
    qreal hour2 = timeToNum(departure);

    qDebug() << "Got" << stId << arrival << departure << platf;

    QPainterPath path;

    const qreal x = 0.0 /*= Session->getStationGraphPos(lineId, stId, platf)*/;
    QPointF cur(x, vertOffset + hour2 * hourOffset);

    path.moveTo(cur);

    //Session->mLineStorage->addJobStop(stopId, stId, m_jobId, lineId, name, cur.y(), cur.y(), platf);

    ++it;

    for(; it != q_selectSegStops.end(); ++it)
    {
        stopId = (*it).get<db_id>(0);
        stId = (*it).get<db_id>(1);
        arrival = (*it).get<QTime>(2);
        departure = (*it).get<QTime>(3);
        platf = (*it).get<int>(4);
        qDebug() << "Got" << stId << arrival << departure << platf;

        hour1 = timeToNum(arrival);
        hour2 = timeToNum(departure);

        QPointF point1(0 /*Session->getStationGraphPos(lineId, stId, platf)*/,
                       vertOffset + hour1 * hourOffset);
        QPointF point2(point1.x(),
                       vertOffset + hour2 * hourOffset);

        //Session->mLineStorage->addJobStop(stopId, stId, m_jobId, lineId, name, point1.y(), point2.y(), platf);

        QPointF midPoint = (cur + point1)/2;

        vec << midPoint;


        path.lineTo(point1);
        path.lineTo(point2);

        if(cur.x() < point1.x())
            angles << true;
        else
            angles << false;

        cur = point2;
    }

    s.item->setPath(path);
    s.item->setToolTip(name);

    //TODO: Do some heuristics on text positioning
    //e.g. Don't write 2 texts if they are too near
    // Or write more than 1 if stops are too distant

    const int oldSize = s.texts.size();
    const int size = vec.size();

    if(oldSize > size)
    {
        auto iter = s.texts.begin() + size;
        auto e = s.texts.end();
        qDeleteAll(iter, e);
        s.texts.erase(iter, e);
        s.texts.squeeze();
    }
    else
    {
        s.texts.reserve(size);
    }

    const int max = qMin(size, oldSize);
    for(int i = 0; i < max; i++)
    {
        QGraphicsSimpleTextItem *ti = s.texts[i];

        ti->setBrush(brush); // Don't set QPen
        ti->setText(name);
        ti->setFont(labelFont);
        const QPointF& p = vec[i];
        if(angles[i])
            ti->setPos(p.x(), p.y() - 25);
        else
            ti->setPos(p.x(), p.y() + 4);
    }

    for(int i = oldSize; i < size; i++)
    {
//        QGraphicsSimpleTextItem *ti = scene->addSimpleText(name,
//                                                           labelFont);
//        ti->setBrush(brush); // Don't set QPen
//        ti->setZValue(1); //Labels must be above jobs and platforms

//        const QPointF& p = vec[i];
//        if(angles[i])
//            ti->setPos(p.x(), p.y() - 25);
//        else
//            ti->setPos(p.x(), p.y() + 4);
//        s.texts << ti;
    }
}

SegmentGraph::SegmentGraph(db_id seg, db_id line) :
    segId(seg),
    lineId(line),
    item(nullptr),
    texts()
{

}

void SegmentGraph::cleanup()
{
    if(item)
    {
        //Pevent an automatic selection of an item that will change JobPathEditor
        //causing maybeSave() and recursion
        //BIG TODO: prevent recursion in JobPathEditor
        //Block setJob() while in saveChanges() or discardChanges()
        delete item;
        item = nullptr;
    }

    qDeleteAll(texts);
    texts.clear();
}
