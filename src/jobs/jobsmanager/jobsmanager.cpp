#include "jobsmanager.h"

#include <QDebug>

#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QToolBar>

#include "model/joblistmodel.h"
#include "model/jobshelper.h"
#include "utils/jobcategorystrings.h"

#include "utils/sqldelegate/modelpageswitcher.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include <QMessageBox>
#include "newjobsamepathdlg.h"
#include "utils/owningqpointer.h"

JobsManager::JobsManager(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *l = new QVBoxLayout(this);
    setMinimumSize(750, 300);

    QToolBar *toolBar = new QToolBar(this);
    toolBar->addAction(tr("New Job"), this, &JobsManager::onNewJob);
    toolBar->addAction(tr("Remove"), this, &JobsManager::onRemove);
    toolBar->addAction(tr("Remove All"), this, &JobsManager::onRemoveAllJobs);
    toolBar->addAction(tr("New Same Path"), this, &JobsManager::onNewJobSamePath);
    l->addWidget(toolBar);

    view = new QTableView;
    connect(view, &QTableView::doubleClicked, this, &JobsManager::onIndexClicked);
    l->addWidget(view);

    jobsModel = new JobListModel(Session->m_Db, this);
    view->setModel(jobsModel);

    auto ps = new ModelPageSwitcher(false, this);
    l->addWidget(ps);
    ps->setModel(jobsModel);
    //Custom colun sorting
    //NOTE: leave disconnect() in the old SIGLAL()/SLOT() version in order to work
    QHeaderView *header = view->horizontalHeader();
    disconnect(header, SIGNAL(sectionPressed(int)), view, SLOT(selectColumn(int)));
    disconnect(header, SIGNAL(sectionEntered(int)), view, SLOT(_q_selectColumn(int)));
    connect(header, &QHeaderView::sectionClicked, this, [this, header](int section)
    {
        jobsModel->setSortingColumn(section);
        header->setSortIndicator(jobsModel->getSortingColumn(), Qt::AscendingOrder);
    });
    header->setSortIndicatorShown(true);
    header->setSortIndicator(jobsModel->getSortingColumn(), Qt::AscendingOrder);

    jobsModel->refreshData();

    setWindowTitle("Jobs Manager");
}

void JobsManager::onIndexClicked(const QModelIndex& index)
{
    db_id jobId = jobsModel->getIdAtRow(index.row());
    if(!jobId)
        return;
    Session->getViewManager()->requestJobEditor(jobId);
}

void JobsManager::onNewJob()
{
    Session->getViewManager()->requestJobCreation();
}

void JobsManager::onRemove()
{
    QModelIndex idx = view->currentIndex();
    if(!idx.isValid())
        return;

    db_id jobId = jobsModel->getIdAtRow(idx.row());
    JobCategory jobCat = jobsModel->getShiftAnCatAtRow(idx.row()).second;
    QString jobName = JobCategoryName::jobName(jobId, jobCat);

    int ret = QMessageBox::question(this,
                                    tr("Job deletion"),
                                    tr("Are you sure to delete job %1?").arg(jobName),
                                    QMessageBox::Yes | QMessageBox::Cancel);
    if(ret == QMessageBox::Yes)
    {
        if(!JobsHelper::removeJob(Session->m_Db, jobId))
        {
            qWarning() << "Error while deleting job:" << jobId << "from JobManager" << Session->m_Db.error_msg();
            //ERRORMSG: message box or statusbar
        }
    }
}

void JobsManager::onRemoveAllJobs()
{
    int ret = QMessageBox::question(this, tr("Delete all jobs?"),
                                    tr("Are you really sure you want to delete all jobs from this session?"));
    if(ret == QMessageBox::Yes)
        JobsHelper::removeAllJobs(Session->m_Db);
}

void JobsManager::onNewJobSamePath()
{
    QModelIndex idx = view->currentIndex();
    if(!idx.isValid())
        return;

    db_id jobId = jobsModel->getIdAtRow(idx.row());
    if(!jobId)
        return;
    JobCategory jobCat = jobsModel->getShiftAnCatAtRow(idx.row()).second;
    auto times = jobsModel->getOrigAndDestTimeAtRow(idx.row());

    OwningQPointer<NewJobSamePathDlg> dlg = new NewJobSamePathDlg(this);
    dlg->setSourceJob(jobId, jobCat, times.first, times.second);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    const QTime newStart = dlg->getNewStartTime();
    const int secsOffset = times.first.secsTo(newStart);

    db_id newJobId = 0;
    if(!JobsHelper::createNewJob(Session->m_Db, newJobId, jobCat))
        return;

    JobsHelper::copyStops(Session->m_Db, jobId, newJobId, secsOffset, dlg->shouldCopyRs());

    //Let user edit newly created job
    Session->getViewManager()->requestJobEditor(newJobId);
}
