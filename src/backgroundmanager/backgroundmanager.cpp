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

void BackgroundManager::startAllCheckers()
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
