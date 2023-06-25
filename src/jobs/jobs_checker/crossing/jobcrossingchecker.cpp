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

#ifdef ENABLE_BACKGROUND_MANAGER

#    include "jobcrossingchecker.h"

#    include "jobcrossingtask.h"
#    include "jobcrossingmodel.h"

#    include "app/session.h"
#    include "viewmanager/viewmanager.h"

#    include "utils/owningqpointer.h"
#    include <QMenu>

JobCrossingChecker::JobCrossingChecker(sqlite3pp::database &db, QObject *parent) :
    IBackgroundChecker(db, parent),
    mDb(db)
{
    eventType   = int(JobCrossingResultEvent::_Type);
    errorsModel = new JobCrossingModel(this);

    connect(Session, &MeetingSession::jobChanged, this, &JobCrossingChecker::onJobChanged);
    connect(Session, &MeetingSession::jobRemoved, this, &JobCrossingChecker::onJobRemoved);
}

QString JobCrossingChecker::getName() const
{
    return tr("Job Crossings");
}

void JobCrossingChecker::clearModel()
{
    static_cast<JobCrossingModel *>(errorsModel)->clear();
}

void JobCrossingChecker::showContextMenu(QWidget *panel, const QPoint &pos,
                                         const QModelIndex &idx) const
{
    const JobCrossingModel *model = static_cast<const JobCrossingModel *>(errorsModel);
    auto item                     = model->getItem(idx);
    if (!item)
        return;

    OwningQPointer<QMenu> menu = new QMenu(panel);

    QAction *showInJobEditor   = new QAction(tr("Show in Job Editor"), menu);
    QAction *showInGraph       = new QAction(tr("Show in graph"), menu);

    menu->addAction(showInJobEditor);
    menu->addAction(showInGraph);

    QAction *act = menu->exec(pos);
    if (act == showInJobEditor)
    {
        Session->getViewManager()->requestJobEditor(item->jobId, item->stopId);
    }
    else if (act == showInGraph)
    {
        // TODO: pass stopId
        Session->getViewManager()->requestJobSelection(item->jobId, true, true);
    }
}

void JobCrossingChecker::sessionLoadedHandler()
{
    if (AppSettings.getCheckCrossingWhenOpeningDB())
        startWorker();
}

IQuittableTask *JobCrossingChecker::createMainWorker()
{
    return new JobCrossingTask(mDb, this, {});
}

void JobCrossingChecker::setErrors(QEvent *e, bool merge)
{
    auto model = static_cast<JobCrossingModel *>(errorsModel);
    auto ev    = static_cast<JobCrossingResultEvent *>(e);
    if (merge)
        model->mergeErrors(ev->results);
    else
        model->setErrors(ev->results);
}

void JobCrossingChecker::onJobChanged(db_id newJobId, db_id oldJobId)
{
    auto model = static_cast<JobCrossingModel *>(errorsModel);
    model->renameJob(oldJobId, newJobId);

    // After renaming check job
    if (AppSettings.getCheckCrossingOnJobEdit())
        startWorker();
}

void JobCrossingChecker::onJobRemoved(db_id jobId)
{
    auto model = static_cast<JobCrossingModel *>(errorsModel);
    model->removeJob(jobId);
}

#endif // ENABLE_BACKGROUND_MANAGER
