#include "shiftgraphholder.h"

#include "shiftscene.h"

#include <QGraphicsSimpleTextItem>
#include <QGraphicsLineItem>
#include "utils/model_roles.h"

#include "shiftgraphobj.h"

#include <QDebug>
#include "app/scopedebug.h"

#include "app/session.h"
#include "utils/jobcategorystrings.h"
#include "lines/linestorage.h"

ShiftGraphHolder::ShiftGraphHolder(database& db, QObject *parent) :
    QObject(parent),
    mScene(new ShiftScene(this)),
    mDb(db),
    q_selectShiftJobs(mDb, "SELECT jobs.id, jobs.category,"
                           " s1.arrival, st1.id, st1.short_name, st1.name,"
                           " s2.departure, st2.id, st2.short_name, st2.name"
                           " FROM jobs"
                           " JOIN stops s1 ON s1.id=jobs.firstStop"
                           " JOIN stops s2 ON s2.id=jobs.lastStop"
                           " JOIN stations st1 ON st1.id=s1.stationId"
                           " JOIN stations st2 ON st2.id=s2.stationId"
                           " WHERE jobs.shiftId=? ORDER BY s1.arrival ASC"),
    hideSameStations(true)
{
    updateShiftGraphOptions();

    connect(&AppSettings, &MRTPSettings::jobColorsChanged,
            this, &ShiftGraphHolder::updateJobColors);

    connect(&AppSettings, &MRTPSettings::shiftGraphOptionsChanged,
            this, &ShiftGraphHolder::updateShiftGraphOptions);

    connect(Session->mLineStorage, &LineStorage::stationNameChanged, this, &ShiftGraphHolder::stationNameChanged);

    connect(Session, &MeetingSession::shiftNameChanged, this, &ShiftGraphHolder::shiftNameChanged);
    connect(Session, &MeetingSession::shiftRemoved, this, &ShiftGraphHolder::shiftRemoved);
    connect(Session, &MeetingSession::shiftJobsChanged, this, &ShiftGraphHolder::shiftJobsChanged);
}

ShiftGraphHolder::~ShiftGraphHolder()
{
    lookUp.clear();
    qDeleteAll(m_shifts);
    m_shifts.clear();
}

ShiftScene *ShiftGraphHolder::scene() const
{
    return mScene;
}

void ShiftGraphHolder::loadShifts()
{
    DEBUG_IMPORTANT_ENTRY;

    lookUp.clear();
    qDeleteAll(m_shifts);
    m_shifts.clear();

    query q_getShifts(mDb, "SELECT COUNT(1) FROM jobshifts");
    q_getShifts.step();
    int count = q_getShifts.getRows().get<int>(0);
    m_shifts.reserve(count);

    q_getShifts.prepare("SELECT id,name FROM jobshifts ORDER BY name");

    QPen linePen(Qt::gray);
    linePen.setWidth(4);

    QLineF backLine(0.0, 0.0,
                    25.0 * hourOffset + horizOffset, 0.0);

    for(auto s : q_getShifts) //TODO: optimize
    {
        db_id shiftId = s.get<db_id>(0);
        QString name = tr("Shift %1").arg(s.get<QString>(1));

        ShiftGraphObj *obj = new ShiftGraphObj(shiftId, mScene);
        obj->setName(name, horizOffset);

        obj->line->setLine(backLine);
        obj->line->setPen(linePen);

        drawShift(obj);

        lookUp.insert(shiftId, obj);
        int i = 0;
        for(; i < m_shifts.size(); i++)
        {
            if(name.compare(m_shifts.at(i)->getName()) < 0)
                break;
        }

        m_shifts.insert(i, obj);

        for(; i < m_shifts.size(); i++)
        {
            setObjPos(m_shifts[i], i);
        }
    }

    m_shifts.squeeze();

    QRectF r = mScene->itemsBoundingRect();
    r.setTopLeft(QPointF(0, 0));
    mScene->setSceneRect(r);
}

void ShiftGraphHolder::shiftNameChanged(db_id shiftId) //TODO: optimize
{
    DEBUG_IMPORTANT_ENTRY;
    auto it = lookUp.constFind(shiftId);
    if(it == lookUp.constEnd())
        return;

    const QString newName = tr("Shift %1").arg(getShiftName(shiftId));

    ShiftGraphObj *obj = it.value();
    int oldPos = obj->pos;

    int res = newName.compare(obj->getName());
    if(res == 0)
    {
        return; //They are equal
    }
    else if(res < 0)
    {
        int i = 0;
        for(; i < oldPos; i++)
        {
            if(newName.compare(m_shifts.at(i)->getName()) < 0)
            {
                break;
            }
        }

        //m_shifts.removeAt(oldPos);
        //m_shifts.insert(i, obj);
        m_shifts.move(oldPos, i); //Small Optimization

        for(; i <= oldPos; i++)
        {
            setObjPos(m_shifts[i], i);
        }
    }
    else
    {
        //m_shifts.removeAt(oldPos);

        int i = oldPos + 1;
        for(; i < m_shifts.size(); i++)
        {
            if(newName.compare(m_shifts.at(i)->getName()) < 0)
            {
                break;
            }

            setObjPos(m_shifts[i], i - 1);
        }

        //m_shifts.insert(i, obj);
        m_shifts.move(oldPos, i - 1); //Small Optimization

        setObjPos(obj, i - 1);
    }

    obj->setName(newName, horizOffset);
}

void ShiftGraphHolder::shiftRemoved(db_id shiftId)
{
    DEBUG_IMPORTANT_ENTRY;
    auto it = lookUp.constFind(shiftId);
    if(it == lookUp.constEnd())
        return;

    ShiftGraphObj *obj = it.value();
    int pos = obj->pos;

    lookUp.erase(it);
    m_shifts.removeAt(pos);
    for(int i = pos; i < m_shifts.size(); i++)
    {
        setObjPos(m_shifts[i], i);
    }

    delete obj;
}

void ShiftGraphHolder::shiftJobsChanged(db_id shiftId)
{
    DEBUG_ENTRY;
    auto it = lookUp.constFind(shiftId);
    if(it == lookUp.constEnd())
        return;

    drawShift(it.value());
}

void ShiftGraphHolder::stationNameChanged(db_id /*stId*/)
{
    //TODO: update only labels
    redrawGraph();
}

void ShiftGraphHolder::drawShift(ShiftGraphObj *obj)
{
    DEBUG_IMPORTANT_ENTRY;

    db_id lastSt = 0;
    int pos = 0;

    obj->clearJobs();
    obj->centerName(horizOffset);

    QPen p;
    p.setWidth(5);

    q_selectShiftJobs.bind(1, obj->shiftId);
    for(auto job : q_selectShiftJobs)
    {
        db_id jobId = job.get<db_id>(0);
        JobCategory cat = JobCategory(job.get<int>(1));

        QTime arr = job.get<QTime>(2);
        db_id st1 = job.get<db_id>(3);
        QString st1_name = job.get<QString>(4);

        //If 'Short Name' is empty fallback to 'Full Name'
        if(st1_name.isEmpty())
            st1_name = job.get<QString>(5);

        QTime dep = job.get<QTime>(6);
        db_id st2 = job.get<db_id>(7);
        QString st2_name = job.get<QString>(8);

        //If 'Short Name' is empty fallback to 'Full Name'
        if(st2_name.isEmpty())
            st2_name = job.get<QString>(9);

        const qreal y = obj->pos * jobOffset + vertOffset;

        ShiftGraphObj::JobGraph g;
        g.pos = pos;

        QLineF l(jobPos(arr), 0, jobPos(dep), 0);

        p.setColor(Session->colorForCat(cat));
        g.line = mScene->addLine(l, p);
        g.line->setY(y + jobOffset / 2);
        g.line->setData(JOB_ID_ROLE, jobId);

        g.name = mScene->addSimpleText(JobCategoryName::jobName(jobId, cat));
        QRectF nameRect = g.name->boundingRect();
        nameRect.moveCenter(l.center());
        g.name->setPos(nameRect.left(), y);

        if(!hideSameStations || lastSt != st1)
        {
            g.st1_label = mScene->addSimpleText(st1_name);
            g.st1_label->setPos(l.x1(), y + stationNameOffset + jobBoxOffset);
        }

        g.st2_label = mScene->addSimpleText(st2_name);
        g.st2_label->setPos(l.x2(), y + stationNameOffset + jobBoxOffset);

        lastSt = st2;

        auto iter = obj->jobs.insert(jobId, g);
        obj->vec.append(iter);

    }
    q_selectShiftJobs.reset();

    obj->jobs.squeeze();
    obj->vec.squeeze();
}

bool ShiftGraphHolder::getHideSameStations() const
{
    return hideSameStations;
}

void ShiftGraphHolder::setHideSameStations(bool value)
{
    hideSameStations = value;
}

void ShiftGraphHolder::redrawGraph()
{
    loadShifts(); //Reload

    for(int i = 0; i < m_shifts.size(); i++)
    {
        setObjPos(m_shifts[i], i);
        drawShift(m_shifts[i]);
    }

    mScene->invalidate(mScene->sceneRect());
}

void ShiftGraphHolder::updateJobColors()
{
    QPen p;
    p.setWidth(5);

    //TODO: maybe store JobCategory inside JobGraph
    query q_getJobCat(mDb, "SELECT category FROM jobs WHERE id=?");

    for(ShiftGraphObj *obj : m_shifts)
    {
        for(auto it : obj->vec)
        {
            db_id jobId = it.key();
            ShiftGraphObj::JobGraph& g = it.value();

            q_getJobCat.bind(1, jobId);
            if(q_getJobCat.step() != SQLITE_ROW)
            {
                //Error: job does not exist!
            }
            JobCategory cat = JobCategory(q_getJobCat.getRows().get<int>(0));

            p.setColor(Session->colorForCat(cat));

            g.line->setPen(p);
        }
    }
}

QString ShiftGraphHolder::getShiftName(db_id shiftId) const
{
    query q_getShiftName(mDb, "SELECT name FROM jobshifts WHERE id=?"); //TODO: same query as above, make string constant
    QString name;
    q_getShiftName.bind(1, shiftId);
    if(q_getShiftName.step() == SQLITE_ROW)
    {
        name = q_getShiftName.getRows().get<QString>(0);
    }else{
        name = "Error";
    }
    q_getShiftName.reset();

    return name;
}

void ShiftGraphHolder::updateShiftGraphOptions()
{
    mScene->hourOffset = hourOffset = AppSettings.getShiftHourOffset();
    mScene->horizOffset = horizOffset = AppSettings.getShiftHorizOffset();
    mScene->vertOffset = vertOffset = AppSettings.getShiftVertOffset();

    jobOffset = AppSettings.getShiftJobOffset();
    jobBoxOffset = AppSettings.getShiftJobBoxOffset();
    stationNameOffset = AppSettings.getShiftStationOffset();
    hideSameStations = AppSettings.getShiftHideSameStations();

    redrawGraph();
}

void ShiftGraphHolder::setObjPos(ShiftGraphObj *o, int pos)
{
    o->pos = pos;

    const qreal y = pos * jobOffset + vertOffset;
    o->text->setY(y + jobBoxOffset);
    o->line->setY(y + jobOffset / 2 + jobBoxOffset);

    for(ShiftGraphObj::JobGraph& g : o->jobs)
    {
        g.line->setY(y + jobOffset / 2);
        g.name->setY(y);

        if(g.st1_label)
            g.st1_label->setY(y + stationNameOffset + jobBoxOffset);

        g.st2_label->setY(y + stationNameOffset + jobBoxOffset);
    }
}
