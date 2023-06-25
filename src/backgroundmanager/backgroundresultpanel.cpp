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

#    include "backgroundresultpanel.h"

#    include "backgroundmanager/ibackgroundchecker.h"
#    include "backgroundresultwidget.h"

#    include "app/session.h"
#    include "backgroundmanager.h"

BackgroundResultPanel::BackgroundResultPanel(QWidget *parent) :
    QTabWidget(parent)
{
    auto bkMgr = Session->getBackgroundManager();
    connect(bkMgr, &BackgroundManager::checkerAdded, this, &BackgroundResultPanel::addChecker);
    connect(bkMgr, &BackgroundManager::checkerRemoved, this, &BackgroundResultPanel::removeChecker);

    for (auto mgr : bkMgr->checkers)
        addChecker(mgr);
}

void BackgroundResultPanel::addChecker(IBackgroundChecker *mgr)
{
    BackgroundResultWidget *w = new BackgroundResultWidget(mgr, this);
    addTab(w, mgr->getName());
}

void BackgroundResultPanel::removeChecker(IBackgroundChecker *mgr)
{
    for (int i = 0; i < count(); i++)
    {
        BackgroundResultWidget *w = static_cast<BackgroundResultWidget *>(widget(i));
        if (w->mgr == mgr)
        {
            removeTab(i);
            delete w;
            return;
        }
    }
}

#endif // ENABLE_BACKGROUND_MANAGER
