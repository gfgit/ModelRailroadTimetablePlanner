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

#include "shiftgraphview.h"

#include "shifts/shiftgraph/model/shiftgraphscene.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include "utils/jobcategorystrings.h"

#include "utils/owningqpointer.h"
#include "shifts/shiftgraph/jobchangeshiftdlg.h"

#include <QHelpEvent>
#include <QToolTip>

#include <QMenu>

ShiftGraphView::ShiftGraphView(QWidget *parent) :
    BasicGraphView(parent)
{
    viewport()->setContextMenuPolicy(Qt::DefaultContextMenu);
}

bool ShiftGraphView::viewportEvent(QEvent *e)
{
    ShiftGraphScene *shiftScene = qobject_cast<ShiftGraphScene *>(scene());

    if (shiftScene && e->type() == QEvent::ToolTip)
    {
        QHelpEvent *ev   = static_cast<QHelpEvent *>(e);

        QPointF scenePos = mapToScene(ev->pos());

        QString shiftName;
        ShiftGraphScene::JobItem item = shiftScene->getJobAt(scenePos, shiftName);
        if (!item.job.jobId)
        {
            QToolTip::hideText();
        }
        else
        {
            QString msg =
              tr("<table><tr>"
                 "<td>Job:</td><td><b>%1</b></td>"
                 "</tr><tr>"
                 "<td>Shift:</td><td><b>%2</b></td>"
                 "</tr><tr>"
                 "<td>From:</td><td><b>%3</b></td><td>at <b>%4</b></td>"
                 "</tr><tr>"
                 "<td>To:</td><td><b>%5</b></td><td>at <b>%6</b></td>"
                 "</tr></table>")
                .arg(JobCategoryName::jobName(item.job.jobId, item.job.category), shiftName,
                     shiftScene->getStationFullName(item.fromStId), item.start.toString("HH:mm"),
                     shiftScene->getStationFullName(item.toStId), item.end.toString("HH:mm"));

            QToolTip::showText(ev->globalPos(), msg, viewport());
        }

        return true;
    }
    else if (shiftScene && e->type() == QEvent::ContextMenu)
    {
        QContextMenuEvent *ev = static_cast<QContextMenuEvent *>(e);

        QPointF scenePos      = mapToScene(ev->pos());

        QString unusedShiftName;
        ShiftGraphScene::JobItem item = shiftScene->getJobAt(scenePos, unusedShiftName);
        if (!item.job.jobId)
            return false;

        QMenu *menu = new QMenu(this);
        menu->addAction(
          tr("Show Job in Graph"), this,
          [item]() { Session->getViewManager()->requestJobSelection(item.job.jobId, true, true); });
        menu->addAction(tr("Show in Job Editor"), this,
                        [item]() { Session->getViewManager()->requestJobEditor(item.job.jobId); });
        menu->addAction(tr("Change Job Shift"), this,
                        [this, item]()
                        {
                            OwningQPointer<JobChangeShiftDlg> dlg =
                              new JobChangeShiftDlg(Session->m_Db, this);
                            dlg->setJob(item.job.jobId);
                            dlg->exec();
                        });

        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(ev->globalPos());

        return true;
    }

    return QAbstractScrollArea::viewportEvent(e);
}
