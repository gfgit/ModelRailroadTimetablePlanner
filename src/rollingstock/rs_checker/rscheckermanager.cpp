#include "rscheckermanager.h"

#ifdef ENABLE_RS_CHECKER

#include <QThreadPool>

#include "app/session.h"

#include "rsworker.h"
#include "rserrortreemodel.h"

RsCheckerManager::RsCheckerManager(QObject *parent) :
    QObject(parent),
    m_mainWorker(nullptr)
{
    errorsModel = new RsErrorTreeModel(this);

    connect(Session, &MeetingSession::rollingStockPlanChanged, this, &RsCheckerManager::onRSPlanChanged);
}

RsCheckerManager::~RsCheckerManager()
{
    if(m_mainWorker)
    {
        m_mainWorker->stop();
        m_mainWorker->cleanup();
        m_mainWorker = nullptr;
    }

    for(RsErrWorker *task : qAsConst(m_workers))
    {
        task->stop();
        task->cleanup();
    }
    m_workers.clear();
}

bool RsCheckerManager::event(QEvent *e)
{
    if(e->type() == RsWorkerProgressEvent::_Type)
    {
        e->setAccepted(true);

        RsWorkerProgressEvent *ev = static_cast<RsWorkerProgressEvent *>(e);

        if(ev->progress == 0)
        {
            emit progressMax(ev->progressMax);
        }

        emit progress(ev->progress);

        return true;
    }
    else if(e->type() == RsWorkerResultEvent::_Type)
    {
        e->setAccepted(true);

        RsWorkerResultEvent *ev = static_cast<RsWorkerResultEvent *>(e);

        if(m_mainWorker && ev->task == m_mainWorker)
        {
            if(!m_mainWorker->wasStopped())
            {
                errorsModel->setErrors(ev->results);
            }

            delete m_mainWorker;
            m_mainWorker = nullptr;

            emit taskFinished();
        }
        else
        {
            int idx = m_workers.indexOf(ev->task);
            if(idx != -1)
            {
                m_workers.removeAt(idx);
                if(!ev->task->wasStopped())
                    errorsModel->mergeErrors(ev->results);

                delete ev->task;
            }
        }

        return true;
    }

    return QObject::event(e);
}

bool RsCheckerManager::startWorker()
{
    if(m_mainWorker)
        return false;

    if(!Session->m_Db.db())
        return false;

    m_mainWorker = new RsErrWorker(Session->m_Db, this, {});

    QThreadPool::globalInstance()->start(m_mainWorker);

    for(RsErrWorker *task : qAsConst(m_workers))
    {
        if(!QThreadPool::globalInstance()->tryTake(task))
            task->stop();
    }

    return true;
}

void RsCheckerManager::abortTasks()
{
    if(m_mainWorker)
    {
        m_mainWorker->stop();
    }

    for(RsErrWorker *task : qAsConst(m_workers))
    {
        task->stop();
    }
}

void RsCheckerManager::checkRs(const QSet<db_id> &rsIds)
{
    if(rsIds.isEmpty() || !Session->m_Db.db())
        return;

    QVector<db_id> vec(rsIds.size());
    for(db_id rsId : rsIds)
        vec.append(rsId);

    RsErrWorker *task = new RsErrWorker(Session->m_Db, this, vec);
    m_workers.append(task);

    QThreadPool::globalInstance()->start(task);
}

RsErrorTreeModel *RsCheckerManager::getErrorsModel() const
{
    return errorsModel;
}

void RsCheckerManager::clearModel()
{
    errorsModel->clear();
}

void RsCheckerManager::onRSPlanChanged(const QSet<db_id> &rsIds)
{
    if(!AppSettings.getCheckRSOnJobEdit())
        return;

    checkRs(rsIds);
}

#endif // ENABLE_RS_CHECKER
