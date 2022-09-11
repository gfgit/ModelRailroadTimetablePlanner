#include "linegraphmanager.h"

#include "linegraphscene.h"
#include "linegraphselectionhelper.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include <QCoreApplication>
#include "utils/worker_event_types.h"

#include <QDebug>

typedef LineGraphScene::PendingUpdate PendingUpdate;

LineGraphManager::LineGraphManager(QObject *parent) :
    QObject(parent),
    activeScene(nullptr),
    m_followJobOnGraphChange(false),
    m_hasScheduledUpdate(false)
{
    auto session = Session;
    //Stations
    connect(session, &MeetingSession::stationNameChanged, this, &LineGraphManager::onStationNameChanged);
    connect(session, &MeetingSession::stationJobsPlanChanged, this, &LineGraphManager::onStationJobPlanChanged);
    connect(session, &MeetingSession::stationTrackPlanChanged, this, &LineGraphManager::onStationTrackPlanChanged);
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
    //Clear update flag before updating in case one operation triggers update
    m_hasScheduledUpdate = false;

    for(LineGraphScene *scene : qAsConst(scenes))
    {
        if(scene->pendingUpdate.testFlag(PendingUpdate::NothingToDo))
            continue; //Skip

        if(scene->pendingUpdate.testFlag(PendingUpdate::FullReload))
        {
            scene->reload();
        }
        else
        {
            if(scene->pendingUpdate.testFlag(PendingUpdate::ReloadJobs))
            {
                scene->reloadJobs();
            }
            if(scene->pendingUpdate.testFlag(PendingUpdate::ReloadStationNames))
            {
                scene->updateStationNames();
            }

            //Manually cleare pending update and trigger redraw
            scene->pendingUpdate = PendingUpdate::NothingToDo;
            emit scene->redrawGraph();
        }
    }
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
    bool found = false;

    for(LineGraphScene *scene : qAsConst(scenes))
    {
        if(scene->pendingUpdate.testFlag(PendingUpdate::FullReload))
            continue; //Already flagged

        if(scene->stations.contains(stationId))
        {
            scene->pendingUpdate.setFlag(PendingUpdate::ReloadStationNames);
            found = true;
        }
    }

    if(found)
        scheduleUpdate();
}

void LineGraphManager::onStationJobPlanChanged(const QSet<db_id>& stationIds)
{
    onStationPlanChanged_internal(stationIds, int(PendingUpdate::ReloadJobs));
}

void LineGraphManager::onStationTrackPlanChanged(const QSet<db_id> &stationIds)
{
    onStationPlanChanged_internal(stationIds, int(PendingUpdate::FullReload));
}

void LineGraphManager::onStationRemoved(db_id stationId)
{
    //A station can be removed only when not connected and no jobs pass through it.
    //So there is no need to update other scenes because no line will contain
    //The removed station
    clearGraphsOfObject(stationId, LineGraphType::SingleStation);
}

void LineGraphManager::onSegmentNameChanged(db_id segmentId)
{
    QString segName;

    for(LineGraphScene *scene : qAsConst(scenes))
    {
        if(scene->pendingUpdate.testFlag(PendingUpdate::FullReload))
            continue; //Already flagged

        if(scene->graphType == LineGraphType::RailwaySegment && scene->graphObjectId == segmentId)
        {
            if(segName.isEmpty())
            {
                //Fetch new name and cache it
                sqlite3pp::query q(scene->mDb, "SELECT name FROM railway_segments WHERE id=?");
                q.bind(1, segmentId);
                if(q.step() != SQLITE_ROW)
                {
                    qWarning() << "Graph: invalid segment ID" << segmentId;
                    return;
                }

                //Store segment name
                segName = q.getRows().get<QString>(0);
            }

            scene->graphObjectName = segName;
            emit scene->graphChanged(int(scene->graphType), scene->graphObjectId, scene);
        }
    }
}

void LineGraphManager::onSegmentStationsChanged(db_id segmentId)
{
    bool found = false;

    for(LineGraphScene *scene : qAsConst(scenes))
    {
        if(scene->pendingUpdate.testFlag(PendingUpdate::FullReload))
            continue; //Already flagged

        if(scene->graphType == LineGraphType::RailwayLine)
        {
            for(const auto& stPos : qAsConst(scene->stationPositions))
            {
                if(stPos.segmentId == segmentId)
                {
                    scene->pendingUpdate.setFlag(PendingUpdate::FullReload);
                    found = true;
                    break;
                }
            }
        }
        else if(scene->graphType == LineGraphType::RailwaySegment && scene->graphObjectId == segmentId)
        {
            scene->pendingUpdate.setFlag(PendingUpdate::FullReload);
            found = true;
        }
    }

    if(found)
        scheduleUpdate();
}

void LineGraphManager::onSegmentRemoved(db_id segmentId)
{
    //A segment can be removed only when is not on any line
    //And when no jobs pass through it.
    //So there is no need to update other line scenes because no line will contain
    //The removed segment
    clearGraphsOfObject(segmentId, LineGraphType::RailwaySegment);
}

void LineGraphManager::onLineNameChanged(db_id lineId)
{
    QString lineName;

    for(LineGraphScene *scene : qAsConst(scenes))
    {
        if(scene->pendingUpdate.testFlag(PendingUpdate::FullReload))
            continue; //Already flagged

        if(scene->graphType == LineGraphType::RailwayLine && scene->graphObjectId == lineId)
        {
            if(lineName.isEmpty())
            {
                //Fetch new name and cache it
                sqlite3pp::query q(scene->mDb, "SELECT name FROM lines WHERE id=?");
                q.bind(1, lineId);
                if(q.step() != SQLITE_ROW)
                {
                    qWarning() << "Graph: invalid line ID" << lineId;
                    return;
                }

                //Store line name
                lineName = q.getRows().get<QString>(0);
            }

            scene->graphObjectName = lineName;
            emit scene->graphChanged(int(scene->graphType), scene->graphObjectId, scene);
        }
    }
}

void LineGraphManager::onLineSegmentsChanged(db_id lineId)
{
    bool found = false;

    for(LineGraphScene *scene : qAsConst(scenes))
    {
        if(scene->pendingUpdate.testFlag(PendingUpdate::FullReload))
            continue; //Already flagged

        if(scene->graphType == LineGraphType::RailwayLine && scene->graphObjectId == lineId)
        {
            scene->pendingUpdate.setFlag(PendingUpdate::FullReload);
            found = true;
        }
    }

    if(found)
        scheduleUpdate();
}

void LineGraphManager::onLineRemoved(db_id lineId)
{
    //Lines do not affect segments and stations
    //So no other scene needs updating
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

void LineGraphManager::onStationPlanChanged_internal(const QSet<db_id> &stationIds, int flag)
{
    bool found = false;

    for(LineGraphScene *scene : qAsConst(scenes))
    {
        if(scene->pendingUpdate.testFlag(PendingUpdate::FullReload)
            || scene->pendingUpdate.testFlag(PendingUpdate(flag)))
            continue; //Already flagged

        for(db_id stationId : stationIds)
        {
            if(scene->stations.contains(stationId))
            {
                scene->pendingUpdate.setFlag(PendingUpdate(flag));
                found = true;
                break;
            }
        }
    }

    if(found)
        scheduleUpdate();
}
