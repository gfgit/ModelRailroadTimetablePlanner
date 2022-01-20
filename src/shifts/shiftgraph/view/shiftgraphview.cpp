#include "shiftgraphview.h"

#include "shifts/shiftgraph/model/shiftgraphscene.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

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

    if(shiftScene && e->type() == QEvent::ToolTip)
    {
        QHelpEvent *ev = static_cast<QHelpEvent *>(e);

        QPointF scenePos = mapToScene(ev->pos());

        QString msg = shiftScene->getTooltipAt(scenePos);

        if(msg.isEmpty())
            QToolTip::hideText();
        else
            QToolTip::showText(ev->globalPos(), msg, viewport());

        return true;
    }
    else if(shiftScene && e->type() == QEvent::ContextMenu)
    {
        QContextMenuEvent *ev = static_cast<QContextMenuEvent *>(e);

        QPointF scenePos = mapToScene(ev->pos());

        JobEntry job = shiftScene->getJobAt(scenePos);
        if(!job.jobId)
            return false;

        QMenu *menu = new QMenu(this);
        menu->addAction(tr("Show Job in Graph"), this, [job]()
                        {
                            Session->getViewManager()->requestJobSelection(job.jobId, true, true);
                        });
        menu->addAction(tr("Show in Job Editor"), this, [job]()
                        {
                            Session->getViewManager()->requestJobEditor(job.jobId);
                        });
        menu->addAction(tr("Change Job Shift"), this, [this, job]()
                        {
                            OwningQPointer<JobChangeShiftDlg> dlg = new JobChangeShiftDlg(Session->m_Db, this);
                            dlg->setJob(job.jobId);
                            dlg->exec();
                        });

        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(ev->globalPos());

        return true;
    }

    return QAbstractScrollArea::viewportEvent(e);
}
