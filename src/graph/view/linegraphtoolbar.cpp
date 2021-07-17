#include "linegraphtoolbar.h"

#include "graph/model/linegraphscene.h"

#include <QComboBox>
#include "utils/sqldelegate/customcompletionlineedit.h"

#include "stations/match_models/stationsmatchmodel.h"
#include "stations/match_models/railwaysegmentmatchmodel.h"
#include "stations/match_models/linesmatchmodel.h"

#include "app/session.h"

#include <QHBoxLayout>

LineGraphToolbar::LineGraphToolbar(QWidget *parent) :
    QWidget(parent),
    m_scene(nullptr),
    matchModel(nullptr),
    oldGraphType(0)
{
    QHBoxLayout *lay = new QHBoxLayout(this);

    graphTypeCombo = new QComboBox;
    lay->addWidget(graphTypeCombo);

    objectCombo = new CustomCompletionLineEdit(nullptr, this);
    lay->addWidget(objectCombo);

    QStringList items;
    items.reserve(4);
    items << tr("No Graph") << tr("Station") << tr("Segment") << tr("Line");
    graphTypeCombo->addItems(items);
    graphTypeCombo->setCurrentIndex(0);

    connect(graphTypeCombo, qOverload<int>(&QComboBox::activated), this, &LineGraphToolbar::onTypeComboActivated);
    connect(objectCombo, &CustomCompletionLineEdit::completionDone, this, &LineGraphToolbar::onCompletionDone);
}

LineGraphToolbar::~LineGraphToolbar()
{
    if(matchModel)
    {
        objectCombo->setModel(nullptr);
        delete matchModel;
        matchModel = nullptr;
    }
}

void LineGraphToolbar::setScene(LineGraphScene *scene)
{
    if(m_scene)
    {
        disconnect(m_scene, &LineGraphScene::graphChanged, this, &LineGraphToolbar::onGraphChanged);
        disconnect(m_scene, &QObject::destroyed, this, &LineGraphToolbar::onSceneDestroyed);
    }
    m_scene = scene;
    if(m_scene)
    {
        connect(m_scene, &LineGraphScene::graphChanged, this, &LineGraphToolbar::onGraphChanged);
        connect(m_scene, &QObject::destroyed, this, &LineGraphToolbar::onSceneDestroyed);
    }
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

    graphTypeCombo->setCurrentIndex(int(type));
    objectCombo->setData(objectId, name);
    oldGraphType = int(type);
}

void LineGraphToolbar::onGraphChanged(int type, db_id objectId)
{
    setupModel(type);
    graphTypeCombo->setCurrentIndex(type);

    QString name;
    if(m_scene && m_scene->getGraphObjectId() == objectId)
        name = m_scene->getGraphObjectName();
    objectCombo->setData(objectId, name);
}

void LineGraphToolbar::onTypeComboActivated(int index)
{
    setupModel(index);
}

void LineGraphToolbar::onCompletionDone()
{
    db_id objectId;
    QString name;
    if(!objectCombo->getData(objectId, name))
        return;

    if(m_scene)
        m_scene->loadGraph(objectId, LineGraphType(graphTypeCombo->currentIndex()));
}

void LineGraphToolbar::onSceneDestroyed()
{
    m_scene = nullptr;
    onGraphChanged(int(LineGraphType::NoGraph), 0); //Clear UI
}

void LineGraphToolbar::setupModel(int type)
{
    if(type != oldGraphType)
    {
        //Clear old model
        if(matchModel)
        {
            objectCombo->setModel(nullptr);
            delete matchModel;
            matchModel = nullptr;
        }

        db_id objectId = 0; //Manually clear line edit
        QString name;
        if(m_scene && type == int(m_scene->getGraphType()))
        {
            //Suggest current graph object
            objectId = m_scene->getGraphObjectId();
            name = m_scene->getGraphObjectName();
        }

        objectCombo->setData(objectId, name);

        switch (LineGraphType(type))
        {
        case LineGraphType::NoGraph:
        default:
        {
            //Prevent recursion on loadGraph() calling back to us
            oldGraphType = type;

            //Clear graph
            if(m_scene)
                m_scene->loadGraph(0, LineGraphType::NoGraph);
            break;
        }
        case LineGraphType::SingleStation:
        {
            StationsMatchModel *m = new StationsMatchModel(Session->m_Db, this);
            m->setFilter(0);
            matchModel = m;
            break;
        }
        case LineGraphType::RailwaySegment:
        {
            RailwaySegmentMatchModel *m = new RailwaySegmentMatchModel(Session->m_Db, this);
            m->setFilter(0, 0, 0);
            matchModel = m;
            break;
        }
        case LineGraphType::RailwayLine:
        {
            LinesMatchModel *m = new LinesMatchModel(Session->m_Db, true, this);
            matchModel = m;
            break;
        }
        }

        if(matchModel)
            objectCombo->setModel(matchModel);
    }
    oldGraphType = type;
}
