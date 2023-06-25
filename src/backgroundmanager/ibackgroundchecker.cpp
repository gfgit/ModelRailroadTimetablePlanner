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

#    include <QThreadPool>
#    include "utils/thread/iquittabletask.h"
#    include "utils/thread/taskprogressevent.h"

#    include "sqlite3pp/sqlite3pp.h"

IBackgroundChecker::IBackgroundChecker(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
    mDb(db)
{
}

bool IBackgroundChecker::event(QEvent *e)
{
    if (e->type() == TaskProgressEvent::_Type)
    {
        e->setAccepted(true);

        TaskProgressEvent *ev = static_cast<TaskProgressEvent *>(e);
        emit progress(ev->progress, ev->progressMax);

        return true;
    }
    else if (e->type() == eventType)
    {
        e->setAccepted(true);

        GenericTaskEvent *ev = static_cast<GenericTaskEvent *>(e);
        if (m_mainWorker && ev->task == m_mainWorker)
        {
            if (!m_mainWorker->wasStopped())
            {
                setErrors(ev, false);
            }

            delete m_mainWorker;
            m_mainWorker = nullptr;

            emit taskFinished();
        }
        else
        {
            int idx = m_workers.indexOf(ev->task);
            if (idx != -1)
            {
                m_workers.removeAt(idx);
                if (!ev->task->wasStopped())
                    setErrors(ev, true);

                delete ev->task;
            }
        }

        return true;
    }

    return QObject::event(e);
}

bool IBackgroundChecker::startWorker()
{
    if (m_mainWorker)
        return false;

    if (!mDb.db())
        return false;

    m_mainWorker = createMainWorker();

    QThreadPool::globalInstance()->start(m_mainWorker);

    for (auto task = m_workers.begin(); task != m_workers.end(); task++)
    {
        if (QThreadPool::globalInstance()->tryTake(*task))
        {
            IQuittableTask *ptr = *task;
            m_workers.erase(task);
            delete ptr;
        }
        else
        {
            (*task)->stop();
        }
    }

    return true;
}

void IBackgroundChecker::abortTasks()
{
    if (m_mainWorker)
    {
        m_mainWorker->stop();
    }

    for (IQuittableTask *task : qAsConst(m_workers))
    {
        task->stop();
    }
}

void IBackgroundChecker::sessionLoadedHandler()
{
    // no-op
}

void IBackgroundChecker::addSubTask(IQuittableTask *task)
{
    m_workers.append(task);
    QThreadPool::globalInstance()->start(task);
}

#endif // ENABLE_BACKGROUND_MANAGER
