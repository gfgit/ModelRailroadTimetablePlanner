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

#include "rscheckermanager.h"

#ifdef ENABLE_BACKGROUND_MANAGER

#    include "app/session.h"
#    include "viewmanager/viewmanager.h"

#    include "rsworker.h"
#    include "rserrortreemodel.h"

#    include <QSet>

#    include "utils/owningqpointer.h"
#    include <QMenu>

RsCheckerManager::RsCheckerManager(sqlite3pp::database &db, QObject *parent) :
    IBackgroundChecker(db, parent)
{
    eventType   = RsWorkerResultEvent::_Type;
    errorsModel = new RsErrorTreeModel(this);

    connect(Session, &MeetingSession::rollingStockPlanChanged, this,
            &RsCheckerManager::onRSPlanChanged);
}

void RsCheckerManager::checkRs(const QSet<db_id> &rsIds)
{
    if (rsIds.isEmpty() || !Session->m_Db.db())
        return;

    QVector<db_id> vec(rsIds.size());
    for (db_id rsId : rsIds)
        vec.append(rsId);

    RsErrWorker *task = new RsErrWorker(Session->m_Db, this, vec);
    addSubTask(task);
}

QString RsCheckerManager::getName() const
{
    return tr("RS Errors");
}

void RsCheckerManager::clearModel()
{
    static_cast<RsErrorTreeModel *>(errorsModel)->clear();
}

void RsCheckerManager::showContextMenu(QWidget *panel, const QPoint &pos,
                                       const QModelIndex &idx) const
{
    const RsErrorTreeModel *model = static_cast<const RsErrorTreeModel *>(errorsModel);
    auto item                     = model->getItem(idx);
    if (!item)
        return;

    OwningQPointer<QMenu> menu = new QMenu(panel);

    QAction *showInJobEditor   = new QAction(tr("Show in Job Editor"), menu);
    QAction *showRsPlan        = new QAction(tr("Show rollingstock plan"), menu);

    menu->addAction(showInJobEditor);
    menu->addAction(showRsPlan);

    QAction *act = menu->exec(pos);
    if (act == showInJobEditor)
    {
        Session->getViewManager()->requestJobEditor(item->job.jobId, item->stopId);
    }
    else if (act == showRsPlan)
    {
        Session->getViewManager()->requestRSInfo(item->rsId);
    }
}

void RsCheckerManager::sessionLoadedHandler()
{
    if (AppSettings.getCheckRSWhenOpeningDB())
        startWorker();
}

void RsCheckerManager::onRSPlanChanged(const QSet<db_id> &rsIds)
{
    if (!AppSettings.getCheckRSOnJobEdit())
        return;

    checkRs(rsIds);
}

IQuittableTask *RsCheckerManager::createMainWorker()
{
    return new RsErrWorker(mDb, this, {});
}

void RsCheckerManager::setErrors(QEvent *e, bool merge)
{
    auto model = static_cast<RsErrorTreeModel *>(errorsModel);
    auto ev    = static_cast<RsWorkerResultEvent *>(e);
    if (merge)
        model->mergeErrors(ev->results);
    else
        model->setErrors(ev->results);
}

#endif // ENABLE_BACKGROUND_MANAGER
