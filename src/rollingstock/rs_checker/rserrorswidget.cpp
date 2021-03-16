#ifdef ENABLE_RS_CHECKER

#include "rserrorswidget.h"

#include <QTreeView>
#include <QHeaderView>
#include <QProgressBar>
#include <QPushButton>

#include <QGridLayout>

#include "app/session.h"

#include "rserrortreemodel.h"

#include "backgroundmanager/backgroundmanager.h"
#include "rscheckermanager.h"

#include <QMenu>
#include <QAction>

#include "viewmanager/viewmanager.h"


RsErrorsWidget::RsErrorsWidget(QWidget *parent) :
    QWidget(parent),
    timerId(0)
{
    view = new QTreeView;
    view->setUniformRowHeights(true);
    view->setSelectionBehavior(QTreeView::SelectRows);

    progressBar = new QProgressBar;
    startBut = new QPushButton(tr("Start"));
    stopBut = new QPushButton(tr("Stop"));

    startBut->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    stopBut->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    auto mgr = Session->getBackgroundManager()->getRsChecker();

    view->setModel(mgr->getErrorsModel());
    view->header()->setStretchLastSection(true);
    view->setSelectionBehavior(QTreeView::SelectRows);

    QGridLayout *grid = new QGridLayout(this);
    grid->addWidget(view, 0, 0, 1, 3);
    grid->addWidget(startBut, 1, 0, 1, 1);
    grid->addWidget(stopBut, 1, 1, 1, 1);
    grid->addWidget(progressBar, 1, 2, 1, 1);

    connect(mgr, &RsCheckerManager::progress, progressBar, &QProgressBar::setValue);
    connect(mgr, &RsCheckerManager::progressMax, this, &RsErrorsWidget::taskProgressMax);
    connect(mgr, &RsCheckerManager::taskFinished, this, &RsErrorsWidget::taskFinished);

    connect(startBut, &QPushButton::clicked, this, &RsErrorsWidget::startTask);
    connect(stopBut, &QPushButton::clicked, this, &RsErrorsWidget::stopTask);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QTreeView::customContextMenuRequested, this, &RsErrorsWidget::showContextMenu);

    setWindowTitle(tr("Rollingstock Errors"));

    progressBar->hide();
}

void RsErrorsWidget::startTask()
{
    progressBar->setValue(0);

    if(Session->getBackgroundManager()->getRsChecker()->startWorker())
    {
        if(timerId)
        {
            killTimer(timerId); //Stop progressBar from hiding in 1 second
            timerId = 0;
        }
    }
}

void RsErrorsWidget::stopTask()
{
    Session->getBackgroundManager()->abortAllTasks();
}

void RsErrorsWidget::taskProgressMax(int max)
{
    progressBar->setMaximum(max);
    progressBar->show();
}

void RsErrorsWidget::taskFinished()
{
    progressBar->setValue(progressBar->maximum());

    if(timerId)
        killTimer(timerId);
    timerId = startTimer(1000); //Hide progressBar after 1 second
}

void RsErrorsWidget::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == timerId)
    {
        killTimer(timerId);
        timerId = 0;
        progressBar->hide();
    }
}

void RsErrorsWidget::showContextMenu(const QPoint& pos)
{
    QModelIndex idx = view->indexAt(pos);
    if(!idx.isValid())
        return;

    const RsErrorTreeModel *model = static_cast<const RsErrorTreeModel *>(view->model());
    auto item = model->getItem(idx);
    if(!item)
        return;

    QMenu menu(this);

    QAction *showInJobEditor = new QAction(tr("Show in Job Editor"), &menu);
    QAction *showRsPlan = new QAction(tr("Show rollingstock plan"), &menu);

    menu.addAction(showInJobEditor);
    menu.addAction(showRsPlan);

    QAction *act = menu.exec(view->viewport()->mapToGlobal(pos));
    if(act == showInJobEditor)
    {
        Session->getViewManager()->requestJobEditor(item->jobId, item->stopId);
    }
    else if(act == showRsPlan)
    {
        Session->getViewManager()->requestRSInfo(item->rsId);
    }
}

#endif // ENABLE_RS_CHECKER
