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

#include "linegraphwidget.h"

#include "graph/model/linegraphscene.h"

#include "linegraphview.h"
#include "linegraphtoolbar.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"
#include "graph/model/linegraphmanager.h"

#include <QVBoxLayout>

LineGraphWidget::LineGraphWidget(QWidget *parent) :
    QWidget(parent),
    m_scene(nullptr)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    toolBar = new LineGraphToolbar(this);

    lay->addWidget(toolBar);

    view = new LineGraphView(this);
    lay->addWidget(view);

    m_scene = new LineGraphScene(Session->m_Db, this);

    //Subscribe to notifications and to session managment
    Session->getViewManager()->getLineGraphMgr()->registerScene(m_scene);
    view->setScene(m_scene);
    toolBar->setScene(m_scene);

    connect(view, &LineGraphView::syncToolbarToScene, toolBar, &LineGraphToolbar::resetToolbarToScene);
    connect(toolBar, &LineGraphToolbar::requestRedraw, m_scene, &LineGraphScene::reload);

    connect(toolBar, &LineGraphToolbar::requestZoom, view, &LineGraphView::setZoomLevel);
    connect(view, &LineGraphView::zoomLevelChanged, toolBar, &LineGraphToolbar::updateZoomLevel);
}

bool LineGraphWidget::tryLoadGraph(db_id graphObjId, LineGraphType type)
{
    if(!m_scene)
        return false;

    return m_scene->loadGraph(graphObjId, type);
}
