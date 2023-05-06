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

#include "backgroundmanager.h"

#ifdef ENABLE_BACKGROUND_MANAGER
#include "backgroundmanager/ibackgroundchecker.h"

#include <QThreadPool>
#include <QSet>

BackgroundManager::BackgroundManager(QObject *parent) :
    QObject(parent)
{

}

BackgroundManager::~BackgroundManager()
{

}

void BackgroundManager::handleSessionLoaded()
{
    for(IBackgroundChecker *mgr : qAsConst(checkers))
        mgr->startWorker();
}

void BackgroundManager::abortAllTasks()
{
    emit abortTrivialTasks();

    for(IBackgroundChecker *mgr : qAsConst(checkers))
        mgr->abortTasks();
}

bool BackgroundManager::isRunning()
{
    bool running = QThreadPool::globalInstance()->activeThreadCount() > 0;
    if(running)
        return true;

    for(IBackgroundChecker *mgr : qAsConst(checkers))
    {
        if(mgr->isRunning())
            return true;
    }

    return false;
}

void BackgroundManager::addChecker(IBackgroundChecker *mgr)
{
    checkers.append(mgr);

    connect(mgr, &IBackgroundChecker::destroyed, this, [this](QObject *self)
            {
                removeChecker(static_cast<IBackgroundChecker *>(self));
            });

    emit checkerAdded(mgr);
}

void BackgroundManager::removeChecker(IBackgroundChecker *mgr)
{
    emit checkerRemoved(mgr);
    disconnect(mgr, nullptr, this, nullptr);
    checkers.removeOne(mgr);
}

void BackgroundManager::clearResults()
{
    for(IBackgroundChecker *mgr : qAsConst(checkers))
    {
        mgr->clearModel();
    }
}

#endif // ENABLE_BACKGROUND_MANAGER
