#include "backgroundmanager.h"

#ifdef ENABLE_BACKGROUND_MANAGER
#include "backgroundmanager/ibackgroundchecker.h"

#include <QThreadPool>
#include "rollingstock/rs_checker/rscheckermanager.h"

#include <QSet>

BackgroundManager::BackgroundManager(QObject *parent) :
    QObject(parent)
{
#ifdef ENABLE_RS_CHECKER
    rsChecker = new RsCheckerManager(this);
#endif
}

BackgroundManager::~BackgroundManager()
{
#ifdef ENABLE_RS_CHECKER
    delete rsChecker;
    rsChecker = nullptr;
#endif
}

void BackgroundManager::abortAllTasks()
{
    emit abortTrivialTasks();

#ifdef ENABLE_RS_CHECKER
    rsChecker->abortTasks();
#endif

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

#ifdef ENABLE_RS_CHECKER
    running |= rsChecker->isRunning();
#endif

    return running;
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
