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

#include "linegraphview.h"

#include "graph/model/linegraphscene.h"

#include "utils/jobcategorystrings.h"

#include "app/session.h"

#include <QToolTip>
#include <QHelpEvent>

LineGraphView::LineGraphView(QWidget *parent) :
    BasicGraphView(parent)
{
}

bool LineGraphView::viewportEvent(QEvent *e)
{
    LineGraphScene *lineScene = qobject_cast<LineGraphScene *>(scene());

    if (e->type() == QEvent::ToolTip && lineScene
        && lineScene->getGraphType() != LineGraphType::NoGraph)
    {
        QHelpEvent *ev         = static_cast<QHelpEvent *>(e);

        const QPointF scenePos = mapToScene(ev->pos());

        JobStopEntry job       = lineScene->getJobAt(scenePos, Session->platformOffset / 2);

        if (job.jobId)
        {
            QToolTip::showText(ev->globalPos(), JobCategoryName::jobName(job.jobId, job.category),
                               viewport());
        }
        else
        {
            QToolTip::hideText();
        }

        return true;
    }

    return QAbstractScrollArea::viewportEvent(e);
}

void LineGraphView::mousePressEvent(QMouseEvent *e)
{
    emit syncToolbarToScene();
    QAbstractScrollArea::mousePressEvent(e);
}

void LineGraphView::mouseDoubleClickEvent(QMouseEvent *e)
{
    LineGraphScene *lineScene = qobject_cast<LineGraphScene *>(scene());

    if (!lineScene || lineScene->getGraphType() == LineGraphType::NoGraph)
        return; // Nothing to select

    const QPointF scenePos = mapToScene(e->pos());

    JobStopEntry job       = lineScene->getJobAt(scenePos, Session->platformOffset / 2);
    lineScene->setSelectedJob(job);
}
