#include "linegraphmanager.h"

#include "linegraphscene.h"
#include "linegraphselectionhelper.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include <QCoreApplication>
#include "utils/worker_event_types.h"

#include <QDebug>

LineGraphManager::LineGraphManager(QObject *parent) :
    QObject(parent),
    activeScene(nullptr),
    m_followJobOnGraphChange(false),
    m_hasScheduledUpdate(false)
{
    auto session = Session;
    //Stations
    connect(session, &MeetingSession::stationNameChanged, this, &LineGraphManager::onStationNameChanged);
    connect(session, &MeetingSession::stationPlanChanged, this, &LineGraphManager::onStationPlanChanged);
    connect(session, &MeetingSession::stationRemoved, this, &LineGraphManager::onStationRemoved);

    //Segments
    connect(session, &MeetingSession::segmentNameChanged, this, &LineGraphManager::onSegmentNameChanged);
    connect(session, &MeetingSession::segmentStationsChanged, this, &LineGraphManager::onSegmentStationsChanged);
    connect(session, &MeetingSession::segmentRemoved, this, &LineGraphManager::onSegmentRemoved);

    //Lines
    connect(session, &MeetingSession::lineNameChanged, this, &LineGraphManager::onLineNameChanged);
    connect(session, &MeetingSession::lineSegmentsChanged, this, &LineGraphManager::onLineSegmentsChanged);
    connect(session, &MeetingSession::lineRemoved, this, &LineGraphManager::onLineRemoved);

    //Jobs
    connect(session, &MeetingSession::jobChanged, this, &LineGraphManager::onJobChanged);

    //Settings
    connect(&AppSettings, &MRTPSettings::jobGraphOptionsChanged, this, &LineGraphManager::updateGraphOptions);
    m_followJobOnGraphChange = AppSettings.getFollowSelectionOnGraphChange();
}

bool LineGraphManager::event(QEvent *ev)
{
    if(ev->type() == QEvent::Type(CustomEvents::LineGraphManagerUpdate))
    {
        ev->accept();
        processPendingUpdates();
        return true;
    }

    return QObject::event(ev);
}

void LineGraphManager::registerScene(LineGraphScene *scene)
{
    Q_ASSERT(!scenes.contains(scene));

    scenes.append(scene);

    connect(scene, &LineGraphScene::destroyed, this, &LineGraphManager::onSceneDestroyed);
    connect(scene, &LineGraphScene::sceneActivated, this, &LineGraphManager::setActiveScene);
    connect(scene, &LineGraphScene::jobSelected, this, &LineGraphManager::onJobSelected);

    if(m_followJobOnGraphChange)
        connect(scene, &LineGraphScene::graphChanged, this, &LineGraphManager::onGraphChanged);

    if(scenes.count() == 1)
    {
        //This is the first scene registered
        //activate it so we have an active scene even if user does't activate one
        setActiveScene(scene);
    }
}

void LineGraphManager::unregisterScene(LineGraphScene *scene)
{
    Q_ASSERT(scenes.contains(scene));

    scenes.removeOne(scene);

    disconnect(scene, &LineGraphScene::destroyed, this, &LineGraphManager::onSceneDestroyed);
    disconnect(scene, &LineGraphScene::sceneActivated, this, &LineGraphManager::setActiveScene);
    disconnect(scene, &LineGraphScene::jobSelected, this, &LineGraphManager::onJobSelected);

    if(m_followJobOnGraphChange)
        disconnect(scene, &LineGraphScene::graphChanged, this, &LineGraphManager::onGraphChanged);

    //Reset active scene if it is unregistered
    if(activeScene == scene)
        setActiveScene(nullptr);
}

void LineGraphManager::clearAllGraphs()
{
    for(LineGraphScene *scene : qAsConst(scenes))
    {
        scene->loadGraph(0, LineGraphType::NoGraph, true);
    }
}

void LineGraphManager::clearGraphsOfObject(db_id objectId, LineGraphType type)
{
    for(LineGraphScene *scene : qAsConst(scenes))
    {
        if(scene->getGraphObjectId() == objectId && scene->getGraphType() == type)
            scene->loadGraph(0, LineGraphType::NoGraph);
    }
}

JobStopEntry LineGraphManager::getCurrentSelectedJob() const
{
    JobStopEntry selectedJob;
    if(activeScene)
        selectedJob = activeScene->getSelectedJob();
    return selectedJob;
}

void LineGraphManager::scheduleUpdate()
{
    if(m_hasScheduledUpdate)
        return; //Already scheduled

    //Mark as scheduled and post event to ourself
    m_hasScheduledUpdate = true;
    QCoreApplication::postEvent(this,
        new QEvent(QEvent::Type(CustomEvents::LineGraphManagerUpdate)),
        Qt::HighEventPriority);
}

void LineGraphManager::processPendingUpdates()
{
    //Clear update flag
    m_hasScheduledUpdate = false;
}

void LineGraphManager::setActiveScene(IGraphScene *scene)
{
    LineGraphScene *lineScene = qobject_cast<LineGraphScene *>(scene);

    if(lineScene)
    {
        if(activeScene == lineScene)
            return;

        //NOTE: Only registere scenes can become active
        //Otherwise we cannot track if scene got destroyed and reset active scene.
        if(!scenes.contains(lineScene))
            return;
    }
    else if(!scenes.isEmpty())
    {
        //Activate first registered scene because previous one was unregistered
        lineScene = scenes.first();
    }

    activeScene = lineScene;
    emit activeSceneChanged(activeScene);

    //Triegger selection update or clear it
    JobStopEntry selectedJob;
    if(activeScene)
    {
        selectedJob = activeScene->getSelectedJob();
    }

    onJobSelected(selectedJob.jobId, int(selectedJob.category), selectedJob.stopId);
}

void LineGraphManager::onSceneDestroyed(QObject *obj)
{
    LineGraphScene *scene = static_cast<LineGraphScene *>(obj);
    unregisterScene(scene);
}

void LineGraphManager::onGraphChanged(int graphType_, db_id graphObjId, LineGraphScene *scene)
{
    if(!m_followJobOnGraphChange || !scenes.contains(scene))
    {
        qWarning() << "LineGraphManager: should not receive graph change for scene" << scene;
        return;
    }

    if(!graphObjId || scene->getGraphType() == LineGraphType::NoGraph)
        return; //No graph selected

    //Graph has changed, ensure selected job is still visible (if possible)
    JobStopEntry selectedJob = scene->getSelectedJob();
    if(!selectedJob.jobId)
        return; //No job selected, nothing to do

    LineGraphSelectionHelper helper(Session->m_Db);

    LineGraphSelectionHelper::SegmentInfo info;
    if(!helper.tryFindJobStopInGraph(scene, selectedJob.jobId, info))
        return; //Cannot find job in current graph, give up

    //Ensure job is visible
    scene->requestShowZone(info.firstStationId, info.segmentId, info.arrivalAndStart, info.departure);
}

void LineGraphManager::onJobSelected(db_id jobId, int category, db_id stopId)
{
    JobCategory cat = JobCategory(category);
    if(lastSelectedJob.jobId == jobId && lastSelectedJob.category == cat && lastSelectedJob.stopId == stopId)
        return; //Selection did not change

    lastSelectedJob.jobId = jobId;
    lastSelectedJob.category = cat;
    lastSelectedJob.stopId = stopId;

    if(jobId)
        Session->getViewManager()->requestJobEditor(jobId);
    else
        Session->getViewManager()->requestClearJob();

    if(AppSettings.getSyncSelectionOnAllGraphs())
    {
        //Sync selection among all registered scenes
        for(LineGraphScene *scene : qAsConst(scenes))
        {
            scene->setSelectedJob(lastSelectedJob);
        }
    }

    if(activeScene)
    {
        const JobStopEntry selectedJob = activeScene->getSelectedJob();
        if(selectedJob.jobId == lastSelectedJob.jobId)
        {
            emit jobSelected(lastSelectedJob.jobId, int(lastSelectedJob.category),
                             lastSelectedJob.stopId);
        }
    }
}

void LineGraphManager::onStationNameChanged(db_id stationId)
{
    onStationPlanChanged({stationId}); //FIXME: update only labels
}

void LineGraphManager::onStationPlanChanged(const QSet<db_id>& stationIds)
{
    //FIXME: speed up with threads???
    for(LineGraphScene *scene : qAsConst(scenes))
    {
        for(db_id stationId : stationIds)
        {
            if(scene->stations.contains(stationId))
            {
                scene->reload();
                break;
            }
        }
    }
}

void LineGraphManager::onStationRemoved(db_id stationId)
{
    clearGraphsOfObject(stationId, LineGraphType::SingleStation);
}

void LineGraphManager::onSegmentNameChanged(db_id segmentId)
{
    onSegmentStationsChanged(segmentId); //FIXME: update only labels
}

void LineGraphManager::onSegmentStationsChanged(db_id segmentId)
{
    //FIXME: speed up with threads???
    for(LineGraphScene *scene : qAsConst(scenes))
    {
        if(scene->graphType == LineGraphType::RailwaySegment && scene->graphObjectId == segmentId)
            scene->reload();
        else if(scene->graphType == LineGraphType::RailwayLine)
            scene->reload(); //FIXME: only if inside this line
        //Single stations are not affected
    }
}

void LineGraphManager::onSegmentRemoved(db_id segmentId)
{
    clearGraphsOfObject(segmentId, LineGraphType::RailwaySegment);
}

void LineGraphManager::onLineNameChanged(db_id lineId)
{
    onLineSegmentsChanged(lineId); //FIXME: update only labels
}

void LineGraphManager::onLineSegmentsChanged(db_id lineId)
{
    //FIXME: speed up with threads???
    for(LineGraphScene *scene : qAsConst(scenes))
    {
        if(scene->graphType == LineGraphType::RailwayLine && scene->graphObjectId == lineId)
            scene->reload();
    }
}

void LineGraphManager::onLineRemoved(db_id lineId)
{
    clearGraphsOfObject(lineId, LineGraphType::RailwayLine);
}

void LineGraphManager::onJobChanged(db_id jobId, db_id oldJobId)
{
    //If job changed ID or category, update all scenes which had it selected

    JobStopEntry selectedJob;
    selectedJob.jobId = jobId;

    LineGraphScene::updateJobSelection(Session->m_Db, selectedJob);

    if(!selectedJob.jobId)
        return; //Invalid job ID

    if(activeScene)
    {
        JobStopEntry oldSelectedJob = activeScene->getSelectedJob();
        if(oldSelectedJob.jobId == oldJobId)
        {
            activeScene->setSelectedJob(selectedJob);
            activeScene->reloadJobs();
        }
    }

    for(LineGraphScene *scene : qAsConst(scenes))
    {
        JobStopEntry oldSelectedJob = scene->getSelectedJob();
        if(oldSelectedJob.jobId == oldJobId)
        {
            scene->setSelectedJob(selectedJob);
            scene->reloadJobs();
        }
    }
}

void LineGraphManager::updateGraphOptions()
{
    //TODO: maybe get rid of theese variables in MeetingSession and always use AppSettings?
    int hourOffset = AppSettings.getHourOffset();
    Session->hourOffset = hourOffset;

    int horizOffset = AppSettings.getHorizontalOffset();
    Session->horizOffset = horizOffset;

    int vertOffset = AppSettings.getVerticalOffset();
    Session->vertOffset = vertOffset;

    Session->stationOffset = AppSettings.getStationOffset();
    Session->platformOffset = AppSettings.getPlatformOffset();

    Session->jobLineWidth = AppSettings.getJobLineWidth();

    //Reload all graphs
    for(LineGraphScene *scene : qAsConst(scenes))
    {
        scene->reload();
    }

    const bool oldVal = m_followJobOnGraphChange;
    m_followJobOnGraphChange = AppSettings.getFollowSelectionOnGraphChange();

    if(m_followJobOnGraphChange != oldVal)
    {
        //Update connections
        for(LineGraphScene *scene : qAsConst(scenes))
        {
            if(m_followJobOnGraphChange)
                connect(scene, &LineGraphScene::graphChanged, this, &LineGraphManager::onGraphChanged);
            else
                disconnect(scene, &LineGraphScene::graphChanged, this, &LineGraphManager::onGraphChanged);
        }
    }
}
