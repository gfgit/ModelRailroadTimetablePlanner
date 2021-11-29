#include "linegraphtoolbar.h"

#include "graph/model/linegraphscene.h"

#include "graph/view/linegraphselectionwidget.h"

#include <QComboBox>
#include <QPushButton>

#include "utils/sqldelegate/customcompletionlineedit.h"

#include "stations/match_models/stationsmatchmodel.h"
#include "stations/match_models/railwaysegmentmatchmodel.h"
#include "stations/match_models/linesmatchmodel.h"

#include "app/session.h"

#include <QEvent>

#include <QHBoxLayout>

LineGraphToolbar::LineGraphToolbar(QWidget *parent) :
    QWidget(parent),
    m_scene(nullptr)
{
    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    selectionWidget = new LineGraphSelectionWidget;
    connect(selectionWidget, &LineGraphSelectionWidget::graphChanged, this, &LineGraphToolbar::onWidgetGraphChanged);
    lay->addWidget(selectionWidget);

    redrawBut = new QPushButton(tr("Redraw"));
    connect(redrawBut, &QPushButton::clicked, this, &LineGraphToolbar::requestRedraw);
    lay->addWidget(redrawBut);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    //Accept focus events by click
    setFocusPolicy(Qt::ClickFocus);

    //Install event filter to catch focus events on children widgets
    for(QObject *child : selectionWidget->children())
    {
        if(child->isWidgetType())
            child->installEventFilter(this);
    }
}

LineGraphToolbar::~LineGraphToolbar()
{

}

void LineGraphToolbar::setScene(LineGraphScene *scene)
{
    if(m_scene)
    {
        disconnect(m_scene, &LineGraphScene::graphChanged, this, &LineGraphToolbar::onSceneGraphChanged);
        disconnect(m_scene, &QObject::destroyed, this, &LineGraphToolbar::onSceneDestroyed);
    }
    m_scene = scene;
    if(m_scene)
    {
        connect(m_scene, &LineGraphScene::graphChanged, this, &LineGraphToolbar::onSceneGraphChanged);
        connect(m_scene, &QObject::destroyed, this, &LineGraphToolbar::onSceneDestroyed);
    }
}

bool LineGraphToolbar::eventFilter(QObject *watched, QEvent *ev)
{
    if(ev->type() == QEvent::FocusIn)
    {
        if(m_scene)
            m_scene->activateScene();
    }

    return QWidget::eventFilter(watched, ev);
}

void LineGraphToolbar::resetToolbarToScene()
{
    LineGraphType type = LineGraphType::NoGraph;
    db_id objectId = 0;
    QString name;

    if(m_scene)
    {
        type = m_scene->getGraphType();
        objectId = m_scene->getGraphObjectId();
        name = m_scene->getGraphObjectName();
    }

    selectionWidget->setGraphType(type);
    selectionWidget->setObjectId(objectId, name);
}

void LineGraphToolbar::onWidgetGraphChanged(int type, db_id objectId)
{
    LineGraphType graphType = LineGraphType(type);
    if(graphType == LineGraphType::NoGraph)
        objectId = 0;

    if(graphType != LineGraphType::NoGraph && !objectId)
        return; //User is still selecting an object

    if(m_scene)
        m_scene->loadGraph(objectId, graphType);
}

void LineGraphToolbar::onSceneGraphChanged(int type, db_id objectId)
{
    selectionWidget->setGraphType(LineGraphType(type));

    QString name;
    if(m_scene && m_scene->getGraphObjectId() == objectId)
        name = m_scene->getGraphObjectName();
    selectionWidget->setObjectId(objectId, name);
}

void LineGraphToolbar::onSceneDestroyed()
{
    m_scene = nullptr;
    resetToolbarToScene(); //Clear UI
}

void LineGraphToolbar::focusInEvent(QFocusEvent *e)
{
    if(m_scene)
        m_scene->activateScene();

    QWidget::focusInEvent(e);
}
