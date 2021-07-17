#include "linegraphviewport.h"

#include "graph/model/linegraphscene.h"

#include "app/session.h"

#include <QPainter>

LineGraphViewport::LineGraphViewport(QWidget *parent) :
    QWidget(parent),
    m_scene(nullptr)
{

}

LineGraphScene *LineGraphViewport::scene() const
{
    return m_scene;
}

void LineGraphViewport::setScene(LineGraphScene *newScene)
{
    if(m_scene)
    {
        disconnect(m_scene, &LineGraphScene::redrawGraph, this, &LineGraphViewport::redrawGraph);
        disconnect(m_scene, &QObject::destroyed, this, &LineGraphViewport::onSceneDestroyed);
    }
    m_scene = newScene;
    if(m_scene)
    {
        connect(m_scene, &LineGraphScene::redrawGraph, this, &LineGraphViewport::redrawGraph);
        connect(m_scene, &QObject::destroyed, this, &LineGraphViewport::onSceneDestroyed);
    }
    update();
}

void LineGraphViewport::redrawGraph()
{
    if(!m_scene || m_scene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to draw

    if(m_scene->stationPositions.isEmpty())
        return;

    const auto entry = m_scene->stationPositions.last();
    const int platfCount = m_scene->stations.value(entry.stationId).platforms.count();

    const int maxWidth = Session->horizOffset + entry.xPos + platfCount * Session->platformOffset;
    if(maxWidth != width())
    {
        const int lastY = Session->vertOffset + Session->hourOffset * 24 + 10;
        resize(maxWidth, lastY);
    }

    update();
}

void LineGraphViewport::paintEvent(QPaintEvent *e)
{
    //TODO: repaint only new regions, not all

    if(!m_scene || m_scene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to draw

    QPainter painter(this);
    paintStations(&painter);
}

void LineGraphViewport::onSceneDestroyed()
{
    m_scene = nullptr;
    update();
}

void LineGraphViewport::paintStations(QPainter *painter)
{
    const QRgb white = qRgb(255, 255, 255);

    const int horizOffset = Session->horizOffset;
    const int vertOffset = Session->vertOffset;
    const int stationOffset = Session->stationOffset;
    const double platfOffset = Session->platformOffset;
    const int lastY = vertOffset + Session->hourOffset * 24 + 10;

    const int width = AppSettings.getPlatformLineWidth();
    const QColor mainPlatfColor = AppSettings.getMainPlatfColor();

    QPen platfPen (mainPlatfColor,  width);

    QPointF top(0, vertOffset);
    QPointF bottom(0, lastY);

    for(const StationGraphObject &st : qAsConst(m_scene->stations))
    {
        top.rx() = bottom.rx() = st.xPos + horizOffset;

        for(const StationGraphObject::PlatformGraph& platf : st.platforms)
        {
            if(platf.color == white)
                platfPen.setColor(mainPlatfColor);
            else
                platfPen.setColor(platf.color);

            painter->setPen(platfPen);

            painter->drawLine(top, bottom);

            top.rx() += platfOffset;
            bottom.rx() += platfOffset;
        }
    }
}
