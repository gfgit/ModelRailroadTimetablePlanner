#ifdef ENABLE_BACKGROUND_MANAGER

#include "backgroundresultpanel.h"

#include "backgroundmanager/ibackgroundchecker.h"
#include "backgroundresultwidget.h"

#include "app/session.h"
#include "backgroundmanager.h"

BackgroundResultPanel::BackgroundResultPanel(QWidget *parent) :
    QTabWidget(parent)
{
    auto bkMgr = Session->getBackgroundManager();
    connect(bkMgr, &BackgroundManager::checkerAdded, this, &BackgroundResultPanel::addChecker);
    connect(bkMgr, &BackgroundManager::checkerRemoved, this, &BackgroundResultPanel::removeChecker);

    for(auto mgr : bkMgr->checkers)
        addChecker(mgr);
}

void BackgroundResultPanel::addChecker(IBackgroundChecker *mgr)
{
    BackgroundResultWidget *w = new BackgroundResultWidget(mgr, this);
    addTab(w, mgr->getName());
}

void BackgroundResultPanel::removeChecker(IBackgroundChecker *mgr)
{
    for(int i = 0; i < count(); i++)
    {
        BackgroundResultWidget *w = static_cast<BackgroundResultWidget *>(widget(i));
        if(w->mgr == mgr)
        {
            removeTab(i);
            delete w;
            return;
        }
    }
}

#endif // ENABLE_BACKGROUND_MANAGER
