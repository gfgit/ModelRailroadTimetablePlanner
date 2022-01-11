#include "stationfreersviewer.h"

#include <QTableView>
#include <QHeaderView>
#include <QTimeEdit>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>

#include "utils/owningqpointer.h"
#include <QMessageBox>
#include <QMenu>

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include "stationfreersmodel.h"

/* Widget to view in a list all free rollingstock pieces at a given time m_time
 * in a given time m_stationId
*/
StationFreeRSViewer::StationFreeRSViewer(QWidget *parent) :
    QWidget(parent)
{
    QGridLayout *lay = new QGridLayout(this);

    refreshBut = new QPushButton(tr("Refresh"));
    lay->addWidget(refreshBut, 0, 0, 1, 2);

    QLabel *l = new QLabel(tr("Time:"));
    lay->addWidget(l, 1, 0);

    timeEdit = new QTimeEdit;
    lay->addWidget(timeEdit, 1, 1);

    prevOpBut = new QPushButton(tr("Previous Operation"));
    lay->addWidget(prevOpBut, 2, 0);

    nextOpBut = new QPushButton(tr("Next Operation"));
    lay->addWidget(nextOpBut, 2, 1);

    view = new QTableView;
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->setSelectionBehavior(QTableView::SelectRows);
    lay->addWidget(view, 3, 0, 1, 2);

    model = new StationFreeRSModel(Session->m_Db, this);
    view->setModel(model);

    connect(refreshBut, &QPushButton::clicked, model, &StationFreeRSModel::reloadData);
    connect(timeEdit, &QTimeEdit::editingFinished, this, &StationFreeRSViewer::onTimeEditingFinished);

    connect(nextOpBut, &QPushButton::clicked, this, &StationFreeRSViewer::goToNext);
    connect(prevOpBut, &QPushButton::clicked, this, &StationFreeRSViewer::goToPrev);

    connect(view, &QTableView::customContextMenuRequested, this, &StationFreeRSViewer::showContextMenu);

    //FIXME: move to FilterHeaderView and IPagedItemModel
    //Custom colun sorting
    //NOTE: leave disconnect() in the old SIGLAL()/SLOT() version in order to work
    QHeaderView *header = view->horizontalHeader();
    disconnect(header, SIGNAL(sectionPressed(int)), view, SLOT(selectColumn(int)));
    disconnect(header, SIGNAL(sectionEntered(int)), view, SLOT(_q_selectColumn(int)));

    connect(header, &QHeaderView::sectionClicked, this, &StationFreeRSViewer::sectionClicked);
    header->setSortIndicatorShown(true);
    header->setSortIndicator(StationFreeRSModel::RSNameCol, Qt::AscendingOrder);

    header->setSectionResizeMode(StationFreeRSModel::FreeFromTimeCol, QHeaderView::ResizeToContents);

    setMinimumSize(100, 200);
}

void StationFreeRSViewer::setStation(db_id stId)
{
    model->setStation(stId);
    updateTitle();
}

void StationFreeRSViewer::updateTitle()
{
    setWindowTitle(tr("Free Rollingstock in %1").arg(model->getStationName()));
}

void StationFreeRSViewer::updateData()
{
    model->reloadData();
}

void StationFreeRSViewer::onTimeEditingFinished()
{
    model->setTime(timeEdit->time());
}

/* void StationFreeRSViewer::goToNext()
 * Find the first operation after m_time.
 * If found set m_time to this new value and rebuild the list.
 * This is useful to jump between operation withuot having to guess times.
*/
void StationFreeRSViewer::goToNext()
{
    QTime time;
    StationFreeRSModel::ErrorCodes err = model->getNextOpTime(time);

    if(err == StationFreeRSModel::DBError)
    {
        QMessageBox::warning(this, tr("Error"),
                             tr("Database error. Try again."));
        return;
    }

    if(err == StationFreeRSModel::NoOperationFound)
    {
        QMessageBox::information(this, tr("No Operation Found"),
                                 tr("No operation found in station %1 after %2!")
                                 .arg(model->getStationName())
                                 .arg(model->getTime().toString("HH:mm")));
        return;
    }

    timeEdit->setTime(time);
    model->setTime(time);
}

/* void StationFreeRSViewer::goToPrev()
 * Find the last operation before m_time.
 * If found set m_time to this new value and rebuild the list.
 * This is useful to jump between operation withuot having to guess times.
 * See 'void StationFreeRSViewer::goToNext()'
*/
void StationFreeRSViewer::goToPrev()
{
    QTime time;
    StationFreeRSModel::ErrorCodes err = model->getPrevOpTime(time);

    if(err == StationFreeRSModel::DBError)
    {
        QMessageBox::warning(this, tr("Error"),
                             tr("Database error. Try again."));
        return;
    }

    if(err == StationFreeRSModel::NoOperationFound)
    {
        QMessageBox::information(this, tr("No Operation Found"),
                                 tr("No operation found in station %1 before %2!")
                                 .arg(model->getStationName())
                                 .arg(model->getTime().toString("HH:mm")));
        return;
    }

    timeEdit->setTime(time);
    model->setTime(time);
}

void StationFreeRSViewer::showContextMenu(const QPoint& pos)
{
    QModelIndex idx = view->indexAt(pos);
    if(!idx.isValid())
        return;

    const StationFreeRSModel::Item *item = model->getItemAt(idx.row());

    OwningQPointer<QMenu> menu = new QMenu(this);
    QAction *showRSPlan = menu->addAction(tr("Show RS Plan"));
    QAction *showFromJobInEditor = menu->addAction(tr("Show Job A in JobEditor"));
    QAction *showToJobInEditor = menu->addAction(tr("Show Job B in JobEditor"));

    showFromJobInEditor->setEnabled(item->fromJob);
    showToJobInEditor->setEnabled(item->toJob);

    QAction *act = menu->exec(view->viewport()->mapToGlobal(pos));
    if(act == showRSPlan)
    {
        Session->getViewManager()->requestRSInfo(item->rsId);
    }
    else if(act == showFromJobInEditor)
    {
        Session->getViewManager()->requestJobEditor(item->fromJob, item->fromStopId);
    }
    else if(act == showToJobInEditor)
    {
        Session->getViewManager()->requestJobEditor(item->toJob, item->toStopId);
    }
}

void StationFreeRSViewer::sectionClicked(int col)
{
    model->sortByColumn(col);
    view->horizontalHeader()->setSortIndicator(model->getSortCol(), Qt::AscendingOrder);
}
