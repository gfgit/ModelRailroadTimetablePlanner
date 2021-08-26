#include "viewmanager.h"

#include "app/session.h"

#include "rsjobviewer.h"
#include "rollingstock/manager/rollingstockmanager.h"

#include "stations/manager/stationsmanager.h"
#include "stations/manager/stationjobview.h"
#include "stations/manager/free_rs_viewer/stationfreersviewer.h"

#include "lines/linestorage.h"

#include "shifts/shiftmanager.h"
#include "shifts/shiftviewer.h"
#include "shifts/shiftgraph/shiftgrapheditor.h"

#include "app/scopedebug.h"

#include "jobs/jobstorage.h"
#include "jobs/jobeditor/jobpatheditor.h"
#include "jobs/jobsmanager.h"

#include "graph/graphmanager.h"

#include "sessionstartendrsviewer.h"

ViewManager::ViewManager(QObject *parent) :
    QObject(parent),
    mGraphMgr(nullptr),
    m_mainWidget(nullptr),
    rsManager(nullptr),
    stManager(nullptr),
    shiftManager(nullptr),
    shiftGraphEditor(nullptr),
    jobEditor(nullptr),
    jobsManager(nullptr),
    sessionRSViewer(nullptr)
{
    mGraphMgr = new GraphManager(this);

    //RollingStock
    connect(Session, &MeetingSession::rollingstockRemoved, this, &ViewManager::onRSRemoved);
    connect(Session, &MeetingSession::rollingStockPlanChanged, this, &ViewManager::onRSPlanChanged);
    connect(Session, &MeetingSession::rollingStockModified, this, &ViewManager::onRSInfoChanged);

    //Stations
    LineStorage *lines = Session->mLineStorage;
    connect(lines, &LineStorage::stationRemoved, this, &ViewManager::onStRemoved);
    connect(lines, &LineStorage::stationNameChanged, this, &ViewManager::onStNameChanged);
    connect(lines, &LineStorage::stationModified, this, &ViewManager::onStPlanChanged);

    //Shifts
    connect(Session, &MeetingSession::shiftNameChanged, this, &ViewManager::onShiftEdited);
    connect(Session, &MeetingSession::shiftRemoved, this, &ViewManager::onShiftRemoved);
    connect(Session, &MeetingSession::shiftJobsChanged, this, &ViewManager::onShiftJobsChanged);

    connect(&AppSettings, &MRTPSettings::jobGraphOptionsChanged, this, &ViewManager::onGraphOptionsChanged);
}

void ViewManager::requestRSInfo(db_id rsId)
{
    DEBUG_ENTRY;
    RSJobViewer *viewer = nullptr;

    auto it = rsHash.constFind(rsId);
    if(it != rsHash.cend())
    {
        viewer = it.value();
    }
    else
    {
        //Create a new viewer
        viewer = createRSViewer(rsId);
    }

    viewer->updateInfo();
    viewer->updatePlan();
    viewer->showNormal();
    viewer->update();
    viewer->raise();
}

RSJobViewer *ViewManager::createRSViewer(db_id rsId)
{
    DEBUG_ENTRY;
    RSJobViewer *viewer = new RSJobViewer(m_mainWidget);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->setWindowFlag(Qt::Window);
    viewer->setRS(rsId);

    viewer->setObjectName(QString("RSJobViewer_%1").arg(rsId));

    rsHash.insert(rsId, viewer);
    connect(viewer, &RSJobViewer::destroyed, this, [this, rsId]()
    {
        rsHash.remove(rsId);
    });

    return viewer;
}

void ViewManager::showRSManager()
{
    DEBUG_ENTRY;

    if(rsManager == nullptr)
    {
        rsManager = new RollingStockManager(m_mainWidget);
        connect(rsManager, &RollingStockManager::destroyed, this, [this]()
        {
            rsManager = nullptr;
        });

        rsManager->setAttribute(Qt::WA_DeleteOnClose);
        rsManager->setWindowFlag(Qt::Window);
    }

    rsManager->showNormal();
    rsManager->update();
    rsManager->raise();
}

void ViewManager::onRSRemoved(db_id rsId)
{
    auto it = rsHash.constFind(rsId);
    if(it != rsHash.cend())
    {
        it.value()->close();
        rsHash.erase(it);
    }
}

void ViewManager::onRSPlanChanged(db_id rsId)
{
    auto it = rsHash.constFind(rsId);
    if(it != rsHash.cend())
    {
        RSJobViewer *viewer = it.value();
        viewer->updatePlan();
        viewer->update();
    }
}

void ViewManager::updateRSPlans(QSet<db_id> set)
{
    for(auto rsId : set)
    {
        auto it = rsHash.constFind(rsId);
        if(it != rsHash.cend())
        {
            RSJobViewer *viewer = it.value();
            viewer->updatePlan();
            viewer->update();
        }
    }
}

void ViewManager::onRSInfoChanged(db_id rsId)
{
    auto it = rsHash.constFind(rsId);
    if(it != rsHash.cend())
    {
        RSJobViewer *viewer = it.value();
        viewer->updateInfo();
        viewer->update();
    }
}

void ViewManager::showStationsManager()
{
    DEBUG_ENTRY;
    if(stManager == nullptr)
    {
        stManager = new StationsManager(m_mainWidget);
        connect(stManager, &StationsManager::destroyed, this, [this]()
        {
            stManager = nullptr;
        });

        stManager->setAttribute(Qt::WA_DeleteOnClose);
        stManager->setWindowFlag(Qt::Window);
    }

    stManager->showNormal();
    stManager->update();
    stManager->raise();
}

void ViewManager::onStRemoved(db_id stId)
{
    auto it = stHash.constFind(stId);
    if(it != stHash.cend())
    {
        it.value()->close();
        stHash.erase(it);
    }

    auto it2 = stRSHash.constFind(stId);
    if(it2 != stRSHash.cend())
    {
        it2.value()->close();
        stRSHash.erase(it2);
    }
}

void ViewManager::onStNameChanged(db_id stId)
{
    //If there is a StationJobViewer window open for this station, update it's title (= new station name)
    auto it = stHash.constFind(stId);
    if(it != stHash.cend())
    {
        it.value()->updateName();
    }

    //Same for StationFreeRSViewer
    auto it2 = stRSHash.constFind(stId);
    if(it2 != stRSHash.cend())
    {
        it2.value()->updateTitle();
    }
}

void ViewManager::onStPlanChanged(db_id stId)
{
    //If there is a StationJobViewer window open for this station, update it's contents
    auto it = stHash.constFind(stId);
    if(it != stHash.cend())
    {
        it.value()->updateJobsList();
    }

    auto it2 = stRSHash.constFind(stId);
    if(it2 != stRSHash.cend())
    {
        it2.value()->updateData();
    }
}

StationJobView* ViewManager::createStJobViewer(db_id stId)
{
    DEBUG_ENTRY;
    StationJobView *viewer = new StationJobView(m_mainWidget);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->setWindowFlag(Qt::Window);
    viewer->setStation(stId);

    viewer->setObjectName(QString("StationJobView_%1").arg(stId));

    stHash.insert(stId, viewer);
    connect(viewer, &StationJobView::destroyed, this, [this, stId]()
    {
        stHash.remove(stId);
    });

    return viewer;
}

void ViewManager::requestStPlan(db_id stId)
{
    DEBUG_ENTRY;
    StationJobView *viewer = nullptr;

    auto it = stHash.constFind(stId);
    if(it != stHash.constEnd())
    {
        viewer = it.value();
    }
    else
    {
        viewer = createStJobViewer(stId);
    }

    viewer->updateName();
    viewer->updateJobsList();

    viewer->showNormal();
    viewer->update();
    viewer->raise();
}

StationFreeRSViewer* ViewManager::createStFreeRSViewer(db_id stId)
{
    DEBUG_ENTRY;
    StationFreeRSViewer *viewer = new StationFreeRSViewer(m_mainWidget);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->setWindowFlag(Qt::Window);
    viewer->setStation(stId);

    viewer->setObjectName(QString("StationFreeRSViewer_%1").arg(stId));

    stRSHash.insert(stId, viewer);
    connect(viewer, &StationFreeRSViewer::destroyed, this, [this, stId]()
    {
        stRSHash.remove(stId);
    });

    return viewer;
}

void ViewManager::requestStFreeRSViewer(db_id stId)
{
    DEBUG_ENTRY;
    StationFreeRSViewer *viewer = nullptr;

    auto it = stRSHash.constFind(stId);
    if(it != stRSHash.constEnd())
    {
        viewer = it.value();
    }
    else
    {
        viewer = createStFreeRSViewer(stId);
    }

    viewer->updateTitle();
    viewer->updateData();

    viewer->showNormal();
    viewer->update();
    viewer->raise();
}

void ViewManager::showShiftManager()
{
    DEBUG_ENTRY;
    if(shiftManager == nullptr)
    {
        shiftManager = new ShiftManager(m_mainWidget);
        connect(shiftManager, &ShiftManager::destroyed, this, [this]()
        {
            shiftManager = nullptr;
        });

        shiftManager->setAttribute(Qt::WA_DeleteOnClose);
        shiftManager->setWindowFlag(Qt::Window);
    }

    //shiftManager->updateModel();

    shiftManager->showNormal();
    shiftManager->update();
    shiftManager->raise();
}

void ViewManager::showJobsManager()
{
    if(jobsManager == nullptr)
    {
        jobsManager = new JobsManager(m_mainWidget);

        connect(jobsManager, &JobsManager::destroyed, this, [this]()
        {
            jobsManager = nullptr;
        });

        jobsManager->setAttribute(Qt::WA_DeleteOnClose);
        jobsManager->setWindowFlag(Qt::Window);
    }

    jobsManager->showNormal();
    jobsManager->raise();
}

void ViewManager::showSessionStartEndRSViewer()
{
    if(!sessionRSViewer)
    {
        sessionRSViewer = new SessionStartEndRSViewer(m_mainWidget);
        connect(sessionRSViewer, &QObject::destroyed, this, [this](){
            sessionRSViewer = nullptr;
        });

        sessionRSViewer->setAttribute(Qt::WA_DeleteOnClose);
        sessionRSViewer->setWindowFlag(Qt::Window);
    }

    sessionRSViewer->showNormal();
    sessionRSViewer->raise();
}

void ViewManager::resumeRSImportation()
{
    RollingStockManager::importRS(true, m_mainWidget);
}

ShiftViewer* ViewManager::createShiftViewer(db_id id)
{
    ShiftViewer *viewer = new ShiftViewer(m_mainWidget);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->setWindowFlag(Qt::Window);

    viewer->setShift(id);

    shiftHash.insert(id, viewer);
    connect(viewer, &ShiftViewer::destroyed, this, [this, id]()
    {
        shiftHash.remove(id);
    });

    return viewer;
}

GraphManager *ViewManager::getGraphMgr() const
{
    return mGraphMgr;
}

void ViewManager::requestShiftViewer(db_id id)
{
    ShiftViewer *viewer = nullptr;
    auto it = shiftHash.constFind(id);
    if(it != shiftHash.constEnd())
    {
        viewer = it.value();
    }
    else
    {
        viewer = createShiftViewer(id);
    }

    viewer->updateJobsModel();

    viewer->showNormal();
    viewer->update();
    viewer->raise();
}

void ViewManager::onShiftRemoved(db_id shiftId)
{
    auto it = shiftHash.constFind(shiftId);
    if(it != shiftHash.constEnd())
    {
        it.value()->close();
        shiftHash.erase(it);
    }
}

void ViewManager::onShiftEdited(db_id shiftId)
{
    auto it = shiftHash.constFind(shiftId);
    if(it != shiftHash.constEnd())
    {
        it.value()->updateName();
    }
}

void ViewManager::onShiftJobsChanged(db_id shiftId)
{
    auto it = shiftHash.constFind(shiftId);
    if(it != shiftHash.constEnd())
    {
        it.value()->updateJobsModel();
    }
}

bool ViewManager::closeEditors()
{
    if(jobEditor && !jobEditor->clearJob())
    {
        return false;
    }

    if(rsManager && !rsManager->close())
    {
        return false;
    }

    qDeleteAll(rsHash);
    rsHash.clear();

    if(stManager && !stManager->close())
    {
        return false;
    }

    qDeleteAll(stHash);
    stHash.clear();

    qDeleteAll(stRSHash);
    stRSHash.clear();


    if(shiftManager && !shiftManager->close())
    {
        return false;
    }

    if(shiftGraphEditor)
    {
        //Delete immidiately so ShiftGraphHolder gets deleted and releases queries
        //Calling 'close()' with WA_DeleteOnClose calls 'deleteLater()' so it gets deleted after MeetingSession tries to close database
        delete shiftGraphEditor;
        shiftGraphEditor = nullptr;
    }

    qDeleteAll(shiftHash);
    shiftHash.clear();

    return true;
}

bool ViewManager::requestJobSelection(db_id jobId, bool select, bool ensureVisible) const
{
    db_id curLineId = mGraphMgr->getCurLineId();

    //Try to select first available segment in current line
    //If the job has no segment in this line, switch current line to first job segment

    db_id segmentId = 0;
    db_id lineId = 0;
    query q(Session->m_Db, "SELECT id,lineId FROM jobsegments WHERE jobId=? ORDER BY num");
    q.bind(1, jobId);
    for(auto r : q)
    {
        db_id segId = r.get<db_id>(0);
        db_id segLineId = r.get<db_id>(1);
        if(!lineId)
        {
            //Save first segment in case we cannot find one in current line
            segmentId = segId;
            lineId = segLineId;
        }
        if(segLineId == curLineId)
        {
            //Fount one in current line, stop search
            segmentId = segId;
            lineId = segLineId;
            break;
        }
    }

    if(!segmentId)
        return false; //Job doesn't seem to have a path, or it doesn't exist!

    //Clear previous selection to avoid multiple selection
    mGraphMgr->clearSelection();

    if(curLineId != lineId)
    {
        //Change current line
        if(!mGraphMgr->setCurrentLine(lineId))
            return false;
    }

    return Session->mJobStorage->selectSegment(jobId, segmentId, select, ensureVisible);
}

//Move to prev/next segment of selected job: changes current line
bool ViewManager::requestJobShowPrevNextSegment(bool prev, bool select, bool ensureVisible)
{
    JobSelection sel = mGraphMgr->getSelectedJob();
    if(sel.jobId == 0 || sel.segmentId == 0)
        return false; //No selected Job

    query q(Session->m_Db);
    if(prev)
    {
        q.prepare("SELECT s.id,s.lineId,MAX(s.num) FROM jobsegments s JOIN jobsegments s1 ON s1.id=?"
                  " WHERE s.jobId=? AND s.num<s1.num");
    }else{
        q.prepare("SELECT s.id,s.lineId,MIN(s.num) FROM jobsegments s JOIN jobsegments s1 ON s1.id=?"
                  " WHERE s.jobId=? AND s.num>s1.num");
    }
    q.bind(1, sel.segmentId);
    q.bind(2, sel.jobId);
    q.step();
    auto r = q.getRows();
    if(r.column_type(0) == SQLITE_NULL)
        return false; //Alredy First segment (or Last if going forward)

    db_id segmentId = r.get<db_id>(0);
    db_id lineId = r.get<db_id>(1);

    //Change current line
    if(!mGraphMgr->setCurrentLine(lineId))
        return false;

    return Session->mJobStorage->selectSegment(sel.jobId, segmentId, select, ensureVisible);
}

bool ViewManager::requestJobEditor(db_id jobId, db_id stopId)
{
    if(!jobEditor || jobId == 0)
        return false;

    if(!jobEditor->setJob(jobId))
        return false;

    jobEditor->parentWidget()->show(); //DockWidget must be visible
    jobEditor->setEnabled(true);
    jobEditor->show();

    if(stopId)
    {
        jobEditor->selectStop(stopId);
    }

    return true;
}

bool ViewManager::requestJobCreation()
{
    /* Creates a new job and opens JobPathEditor */

    if(!jobEditor || !jobEditor->createNewJob())
        return false; //JobPathEditor is busy, abort

    jobEditor->parentWidget()->show(); //DockWidget must be visible
    jobEditor->setEnabled(true);
    jobEditor->show();

    return true;
}

bool ViewManager::requestClearJob(bool evenIfEditing)
{
    if(!jobEditor)
        return false;

    if(jobEditor->isEdited() && !evenIfEditing)
        return false;

    if(!jobEditor->clearJob())
        return false;

    jobEditor->setEnabled(false);
    return true;
}

bool ViewManager::removeSelectedJob()
{
    db_id jobId = mGraphMgr->getSelectedJob().jobId;
    if(jobId == 0)
        return false;

    if(jobEditor)
    {
        if(jobEditor->currentJobId() == jobId)
        {
            if(!jobEditor->clearJob())
            {
                requestJobSelection(jobId, true, false);
                return false;
            }
            jobEditor->setEnabled(false);
        }
    }

    Session->mJobStorage->removeJob(jobId);

    return true;
}

void ViewManager::requestShiftGraphEditor()
{
    if(shiftGraphEditor == nullptr)
    {
        shiftGraphEditor = new ShiftGraphEditor(m_mainWidget);

        connect(shiftGraphEditor, &ShiftGraphEditor::destroyed, this, [this]()
        {
            shiftGraphEditor = nullptr;
        });

        shiftGraphEditor->setAttribute(Qt::WA_DeleteOnClose);
        shiftGraphEditor->setWindowFlag(Qt::Window);
    }

    shiftGraphEditor->showNormal();
    shiftGraphEditor->update();
    shiftGraphEditor->raise();
}

void ViewManager::prepareQueries()
{
    if(jobEditor)
        jobEditor->prepareQueries();
}

void ViewManager::finalizeQueries()
{
    if(jobEditor)
        jobEditor->finalizeQueries();
}

void ViewManager::onGraphOptionsChanged()
{
    //TODO: remove this function, handle directly inside the models, draw line before jobs
    Session->mLineStorage->redrawAllLines();
    Session->mJobStorage->drawJobs();
}
