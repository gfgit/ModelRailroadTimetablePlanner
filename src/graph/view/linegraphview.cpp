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

    if(e->type() == QEvent::ToolTip && lineScene && lineScene->getGraphType() != LineGraphType::NoGraph)
    {
        QHelpEvent *ev = static_cast<QHelpEvent *>(e);

        const QPointF scenePos = mapToScene(ev->pos());

        JobStopEntry job = lineScene->getJobAt(scenePos, Session->platformOffset / 2);

        if(job.jobId)
        {
            QToolTip::showText(ev->globalPos(),
                               JobCategoryName::jobName(job.jobId, job.category),
                               viewport());
        }else{
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

    if(!lineScene || lineScene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to select

    const QPointF scenePos = mapToScene(e->pos());

    JobStopEntry job = lineScene->getJobAt(scenePos, Session->platformOffset / 2);
    lineScene->setSelectedJob(job);
}
