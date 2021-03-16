#include "backgroundmanager.h"

#ifdef ENABLE_BACKGROUND_MANAGER

#include "app/session.h"

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
}

bool BackgroundManager::isRunning()
{
    bool running = QThreadPool::globalInstance()->activeThreadCount() > 0;
    if(running)
        return true;

#ifdef ENABLE_RS_CHECKER
    running |= rsChecker->isRunning();
#endif

    return running;
}

#endif // ENABLE_BACKGROUND_MANAGER
