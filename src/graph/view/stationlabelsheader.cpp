#include "stationlabelsheader.h"

#include "graph/model/linegraphscene.h"

#include "backgroundhelper.h"

#include "app/session.h"

#include <QPainter>

StationLabelsHeader::StationLabelsHeader(QWidget *parent) :
    QWidget(parent),
    m_scene(nullptr),
    horizontalScroll(0)
{

}

void StationLabelsHeader::setScroll(int value)
{
    horizontalScroll = value;
    update();
}

void StationLabelsHeader::onSceneDestroyed()
{
    m_scene = nullptr;
    update();
}

void StationLabelsHeader::paintEvent(QPaintEvent *)
{
    //TODO: repaint only new regions, not all

    if(!m_scene || m_scene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to draw

    QPainter painter(this);
    QColor c(255, 255, 255, 220);
    painter.fillRect(rect(), c);

    BackgroundHelper::drawStationHeader(&painter, m_scene, rect(), horizontalScroll);
}

LineGraphScene *StationLabelsHeader::scene() const
{
    return m_scene;
}

void StationLabelsHeader::setScene(LineGraphScene *newScene)
{
    if(m_scene)
    {
        disconnect(m_scene, &LineGraphScene::redrawGraph, this, qOverload<>(&StationLabelsHeader::update));
        disconnect(m_scene, &QObject::destroyed, this, &StationLabelsHeader::onSceneDestroyed);
    }
    m_scene = newScene;
    if(m_scene)
    {
        connect(m_scene, &LineGraphScene::redrawGraph, this, qOverload<>(&StationLabelsHeader::update));
        connect(m_scene, &QObject::destroyed, this, &StationLabelsHeader::onSceneDestroyed);
    }
    update();
}
