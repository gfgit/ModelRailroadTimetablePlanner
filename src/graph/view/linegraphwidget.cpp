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
}

bool LineGraphWidget::tryLoadGraph(db_id graphObjId, LineGraphType type)
{
    if(!m_scene)
        return false;

    return m_scene->loadGraph(graphObjId, type);
}
