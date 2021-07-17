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
    m_scene = newScene;
    update();
}

void LineGraphViewport::paintEvent(QPaintEvent *event)
{
    //TODO: repaint only new regions, not all

    if(!m_scene || m_scene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to draw

    QPainter painter(this);
    paintStations(&painter);
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

    QPointF top(vertOffset, rect().top());
    QPointF bottom(lastY, rect().bottom());

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
