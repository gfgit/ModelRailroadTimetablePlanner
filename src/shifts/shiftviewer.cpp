#include "shiftviewer.h"

#include "app/session.h"

#include "shiftjobsmodel.h"

#include <QTableView>
#include <QVBoxLayout>

#include "utils/owningqpointer.h"
#include <QMenu>

#include "viewmanager/viewmanager.h"

ShiftViewer::ShiftViewer(QWidget *parent) :
    QWidget(parent),
    shiftId(0),
    q_shiftName(Session->m_Db, "SELECT name FROM jobshifts WHERE id=?")
{
    QVBoxLayout *l = new QVBoxLayout(this);

    view = new QTableView(this);
    l->addWidget(view);

    setMinimumSize(600, 200);

    model = new ShiftJobsModel(Session->m_Db, this);
    view->setModel(model);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QTableView::customContextMenuRequested, this, &ShiftViewer::showContextMenu);
}

ShiftViewer::~ShiftViewer()
{

}

void ShiftViewer::updateName()
{
    q_shiftName.bind(1, shiftId);
    if(q_shiftName.step() != SQLITE_ROW)
    {
        //Error
    }
    else
    {
        setWindowTitle(q_shiftName.getRows().get<QString>(0));
    }
    q_shiftName.reset();
}

void ShiftViewer::setShift(db_id id)
{
    shiftId = id;
    updateName();
}

void ShiftViewer::updateJobsModel()
{
    model->loadShiftJobs(shiftId);
}

void ShiftViewer::showContextMenu(const QPoint& pos)
{
    QModelIndex idx = view->indexAt(pos);
    if(!idx.isValid())
        return;

    db_id jobId = model->getJobAt(idx.row());

    OwningQPointer<QMenu> menu = new QMenu(this);

    QAction *showInJobEditor = new QAction(tr("Show in Job Editor"), menu);
    menu->addAction(showInJobEditor);

    QAction *act = menu->exec(view->viewport()->mapToGlobal(pos));
    if(act == showInJobEditor)
    {
        //TODO: requestJobEditor() doesn't select item in graph
        Session->getViewManager()->requestJobSelection(jobId, true, true);
    }
}
