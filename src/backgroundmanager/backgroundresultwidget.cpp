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

#    include "ibackgroundchecker.h"
#    include "backgroundresultwidget.h"

#    include <QTreeView>
#    include <QHeaderView>
#    include <QProgressBar>
#    include <QPushButton>

#    include <QGridLayout>

#    include <QMenu>
#    include <QAction>

#    include <QTimerEvent>

BackgroundResultWidget::BackgroundResultWidget(IBackgroundChecker *mgr_, QWidget *parent) :
    QWidget(parent),
    mgr(mgr_),
    timerId(0)
{
    view = new QTreeView;
    view->setUniformRowHeights(true);
    view->setSelectionBehavior(QTreeView::SelectRows);

    progressBar = new QProgressBar;
    startBut    = new QPushButton(tr("Start"));
    stopBut     = new QPushButton(tr("Stop"));

    startBut->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    stopBut->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    view->setModel(mgr_->getModel());
    view->header()->setStretchLastSection(true);
    view->setSelectionBehavior(QTreeView::SelectRows);

    QGridLayout *grid = new QGridLayout(this);
    grid->addWidget(view, 0, 0, 1, 3);
    grid->addWidget(startBut, 1, 0, 1, 1);
    grid->addWidget(stopBut, 1, 1, 1, 1);
    grid->addWidget(progressBar, 1, 2, 1, 1);

    connect(mgr, &IBackgroundChecker::progress, this, &BackgroundResultWidget::onTaskProgress);
    connect(mgr, &IBackgroundChecker::taskFinished, this, &BackgroundResultWidget::taskFinished);

    connect(startBut, &QPushButton::clicked, this, &BackgroundResultWidget::startTask);
    connect(stopBut, &QPushButton::clicked, this, &BackgroundResultWidget::stopTask);

    view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view, &QTreeView::customContextMenuRequested, this,
            &BackgroundResultWidget::showContextMenu);

    setWindowTitle(tr("Error Results"));

    progressBar->hide();
}

void BackgroundResultWidget::startTask()
{
    progressBar->setValue(0);

    if (mgr->startWorker())
    {
        if (timerId)
        {
            killTimer(timerId); // Stop progressBar from hiding in 1 second
            timerId = 0;
        }
    }
}

void BackgroundResultWidget::stopTask()
{
    mgr->abortTasks();
}

void BackgroundResultWidget::onTaskProgress(int val, int max)
{
    progressBar->setMaximum(max);
    progressBar->setValue(val);
    progressBar->show();
}

void BackgroundResultWidget::taskFinished()
{
    progressBar->setValue(progressBar->maximum());

    if (timerId)
        killTimer(timerId);
    timerId = startTimer(1000); // Hide progressBar after 1 second
}

void BackgroundResultWidget::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == timerId)
    {
        killTimer(timerId);
        timerId = 0;
        progressBar->hide();
    }
}

void BackgroundResultWidget::showContextMenu(const QPoint &pos)
{
    QModelIndex idx = view->indexAt(pos);
    if (!idx.isValid())
        return;

    mgr->showContextMenu(this, view->viewport()->mapToGlobal(pos), idx);
}

#endif // ENABLE_BACKGROUND_MANAGER
