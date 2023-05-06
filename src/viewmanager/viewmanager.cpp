/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

﻿#include "viewmanager.h"

#include "app/session.h"

#include "rsjobviewer.h"
#include "rollingstock/manager/rollingstockmanager.h"

#include "stations/manager/stationsmanager.h"
#include "stations/manager/stationjobview.h"
#include "stations/manager/free_rs_viewer/stationfreersviewer.h"
#include "stations/manager/stations/dialogs/stationsvgplandlg.h"

#include "shifts/shiftmanager.h"
#include "shifts/shiftviewer.h"
#include "shifts/shiftgraph/shiftgrapheditor.h"

#include "app/scopedebug.h"

#include "jobs/jobeditor/jobpatheditor.h"
#include "jobs/jobsmanager/jobsmanager.h"
#include "jobs/jobsmanager/model/jobshelper.h"
#include "utils/jobcategorystrings.h"

#include "graph/model/linegraphmanager.h"
#include "graph/model/linegraphselectionhelper.h"

#include "sessionstartendrsviewer.h"

#include <QMessageBox>

ViewManager::ViewManager(QObject *parent) :
    QObject(parent),
    m_mainWidget(nullptr),
    rsManager(nullptr),
    sessionRSViewer(nullptr),
    stManager(nullptr),
    shiftManager(nullptr),
    shiftGraphEditor(nullptr),
    jobsManager(nullptr),
    jobEditor(nullptr)
{
    lineGraphManager = new LineGraphManager(this);

    //RollingStock
    connect(Session, &MeetingSession::rollingstockRemoved, this, &ViewManager::onRSRemoved);
    connect(Session, &MeetingSession::rollingStockPlanChanged, this, &ViewManager::onRSPlanChanged);
    connect(Session, &MeetingSession::rollingStockModified, this, &ViewManager::onRSInfoChanged);

    //Stations
    connect(Session, &MeetingSession::stationRemoved, this, &ViewManager::onStRemoved);
    connect(Session, &MeetingSession::stationNameChanged, this, &ViewManager::onStNameChanged);
    connect(Session, &MeetingSession::stationJobsPlanChanged, this, &ViewManager::onStPlanChanged);

    //Jobs
    connect(Session, &MeetingSession::jobRemoved, this, &ViewManager::onJobRemoved);

    //Shifts
    connect(Session, &MeetingSession::shiftNameChanged, this, &ViewManager::onShiftEdited);
    connect(Session, &MeetingSession::shiftRemoved, this, &ViewManager::onShiftRemoved);
    connect(Session, &MeetingSession::shiftJobsChanged, this, &ViewManager::onShiftJobsChanged);
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

void ViewManager::onRSPlanChanged(QSet<db_id> set)
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

    auto it3 = stPlanHash.constFind(stId);
    if(it3 != stPlanHash.cend())
    {
        it3.value()->close();
        stPlanHash.erase(it3);
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

    auto it3 = stPlanHash.constFind(stId);
    if(it3 != stPlanHash.cend())
    {
        it3.value()->reloadDBData();
    }
}

void ViewManager::onStPlanChanged(const QSet<db_id>& stationIds)
{
    //If there is a StationJobViewer window open for this station, update it's contents
    for(db_id stationId : stationIds)
    {
        auto it = stHash.constFind(stationId);
        if(it != stHash.cend())
        {
            it.value()->updateJobsList();
        }

        auto it2 = stRSHash.constFind(stationId);
        if(it2 != stRSHash.cend())
        {
            it2.value()->updateData();
        }

        auto it3 = stPlanHash.constFind(stationId);
        if(it3 != stPlanHash.cend())
        {
            it3.value()->reloadDBData();
        }
    }
}

void ViewManager::onJobRemoved(db_id jobId)
{
    //We already catch normal job removal with other signals
    if(jobId)
        return;

    //If jobId is zero, it means all jobs have been deleted
    //Reload all views, Shift Graph Editor already reloads itself

    closeJobRelatedViewsHelper();

    //Reload station plans
    for(auto st : qAsConst(stPlanHash))
    {
        st->clearJobs();
        st->update();
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

StationSVGPlanDlg *ViewManager::createStPlanDlg(db_id stId, QString& stNameOut)
{
    if(!StationSVGPlanDlg::stationHasSVG(Session->m_Db, stId, &stNameOut))
        return nullptr;

    StationSVGPlanDlg *viewer = new StationSVGPlanDlg(Session->m_Db, m_mainWidget);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->setWindowFlag(Qt::Window);
    viewer->setStation(stId);
    viewer->reloadPlan();

    viewer->setObjectName(QString("StationSVGPlanDlg_%1").arg(stId));

    stPlanHash.insert(stId, viewer);
    connect(viewer, &StationSVGPlanDlg::destroyed, this, [this, stId]()
    {
        stPlanHash.remove(stId);
    });

    return viewer;
}

void ViewManager::requestStJobViewer(db_id stId)
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

void ViewManager::requestStSVGPlan(db_id stId, bool showJobs, const QTime &time)
{
    DEBUG_ENTRY;
    StationSVGPlanDlg *viewer = nullptr;

    auto it = stPlanHash.constFind(stId);
    if(it != stPlanHash.constEnd())
    {
        viewer = it.value();
    }
    else
    {
        QString stName;
        viewer = createStPlanDlg(stId, stName);
        if(!viewer)
        {
            QMessageBox::warning(m_mainWidget, stName,
                                 tr("Station %1 has no SVG, please add one.").arg(stName));
            return;
        }
    }

    if(showJobs)
    {
        viewer->setJobTime(time);
        viewer->showJobs(true);
    }

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

void ViewManager::closeJobRelatedViewsHelper()
{
    //Close all Job related views except managers and Shift Graph Editor
    //SVG station plans are left open because they are useful also for other purposes

    //NOTE: because views destroyed() signals are connected to lambda
    //which remove them from the list, they invalidate iterators.
    //To avoid this we copy the list (swap) and iterate from copy

    //Close all rollingstock views because we have no couplings anymore
    auto rsHashCopy = std::move(rsHash);
    qDeleteAll(rsHashCopy);

    //Close all station job view
    auto stHashCopy = std::move(stHash);
    qDeleteAll(stHashCopy);

    //Close all station free RS view
    auto stRSHashCopy = std::move(stRSHash);
    qDeleteAll(stRSHashCopy);

    //Close any shift job view
    auto shiftHashCopy = std::move(shiftHash);
    qDeleteAll(shiftHashCopy);
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
    if(jobEditor)
    {
        if(!jobEditor->clearJob())
        {
            return false;
        }
        jobEditor->setEnabled(false);
    }

    if(jobsManager && !jobsManager->close())
    {
        return false;
    }

    if(rsManager && !rsManager->close())
    {
        return false;
    }

    if(stManager && !stManager->close())
    {
        return false;
    }

    if(shiftManager && !shiftManager->close())
    {
        return false;
    }

    closeJobRelatedViewsHelper();

    //Also close SVG Station Plans now
    //NOTE: use copy to avoid using invalid iterators, see closeJobRelatedViewsHelper()
    auto stPlanCopy = std::move(stPlanHash);
    qDeleteAll(stPlanCopy);

    if(shiftGraphEditor)
    {
        //Delete immidiately so ShiftGraphHolder gets deleted and releases queries
        //Calling 'close()' with WA_DeleteOnClose calls 'deleteLater()' so it gets deleted after MeetingSession tries to close database
        delete shiftGraphEditor;
        shiftGraphEditor = nullptr;
    }

    return true;
}

void ViewManager::clearAllLineGraphs()
{
    lineGraphManager->clearAllGraphs();
}

bool ViewManager::requestJobSelection(db_id jobId, bool select, bool ensureVisible) const
{
    LineGraphScene *scene = lineGraphManager->getActiveScene();
    if(!scene)
        return false; //No active scene, we cannot select anything

    LineGraphSelectionHelper helper(Session->m_Db);
    return helper.requestJobSelection(scene, jobId, select, ensureVisible);
}

bool ViewManager::requestJobShowPrevNextSegment(bool prev, bool absolute)
{
    LineGraphScene *scene = lineGraphManager->getActiveScene();
    if(!scene)
        return false; //No active scene, we cannot select anything

    LineGraphSelectionHelper helper(Session->m_Db);

    if(prev)
        return helper.requestCurrentJobPrevSegmentVisible(scene, absolute);
    else
        return helper.requestCurrentJobNextSegmentVisible(scene, absolute);
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

    if(!JobsHelper::checkShiftsExist(Session->m_Db))
    {
        //It's better to create Shifts before creating Jobs,
        //remind the user on every new Job
        int ret = QMessageBox::question(m_mainWidget, tr("Create Shift too?"),
                                        tr("No Shift are present in this session.\n"
                                           "Do you want to create some before creating Jobs?"),
                                        QMessageBox::Yes | QMessageBox::No);
        if(ret == QMessageBox::Yes)
        {
            //Create shift
            showShiftManager();
            return false;
        }
    }

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
    JobStopEntry selectedJob = lineGraphManager->getCurrentSelectedJob();
    if(selectedJob.jobId == 0)
        return false;

    //Ask user confirmation
    int ret = QMessageBox::question(m_mainWidget, tr("Remove Job?"),
                                    tr("Are you sure you want to remove Job <b>%1</b>?")
                                        .arg(JobCategoryName::jobName(selectedJob.jobId, selectedJob.category)));
    if(ret != QMessageBox::Yes)
        return false;

    if(jobEditor)
    {
        if(jobEditor->currentJobId() == selectedJob.jobId)
        {
            if(!jobEditor->clearJob())
            {
                requestJobSelection(selectedJob.jobId, true, false);
                return false;
            }
            jobEditor->setEnabled(false);
        }
    }

    return JobsHelper::removeJob(Session->m_Db, selectedJob.jobId);
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
