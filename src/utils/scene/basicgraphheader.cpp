#include "basicgraphheader.h"

#include "igraphscene.h"

#include <QPainter>
#include <QPaintEvent>

BasicGraphHeader::BasicGraphHeader(Qt::Orientation orient, QWidget *parent) :
    QWidget(parent),
    m_scene(nullptr),
    m_scroll(0),
    mZoom(100),
    m_orientation(orient)
{

}

void BasicGraphHeader::setScene(IGraphScene *scene)
{
    m_scene = scene;
}

void BasicGraphHeader::setScroll(int value)
{
    m_scroll = value;
    update();
}

void BasicGraphHeader::setZoom(int value)
{
    mZoom = value;
    update();
}

void BasicGraphHeader::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    QColor c(255, 255, 255, 220);
    painter.fillRect(rect(), c);

    if(!m_scene)
        return; //Nothing to draw

    //FIXME: consider QPaintEvent rect?

    const double scaleFactor = mZoom / 100.0;

    //Map to scene
    const double sceneScroll = m_scroll / scaleFactor;
    QRectF sceneRect = rect();
    sceneRect.setSize(sceneRect.size() / scaleFactor);

    //Apply scrolling
    if(m_orientation == Qt::Horizontal)
        sceneRect.moveLeft(sceneScroll);
    else
        sceneRect.moveTop(sceneScroll);

    painter.scale(scaleFactor, scaleFactor);
    painter.translate(-sceneRect.topLeft());

    m_scene->renderHeader(&painter, sceneRect, m_orientation, sceneScroll);
}
