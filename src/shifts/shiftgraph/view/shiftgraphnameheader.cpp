#include "shiftgraphnameheader.h"

#include "shifts/shiftgraph/model/shiftgraphscene.h"

#include <QPainter>
#include <QPaintEvent>

ShiftGraphNameHeader::ShiftGraphNameHeader(QWidget *parent) :
    QWidget(parent),
    m_scene(nullptr),
    verticalScroll(0),
    mZoom(100)
{

}

void ShiftGraphNameHeader::setScene(ShiftGraphScene *newScene)
{
    m_scene = newScene;
}

void ShiftGraphNameHeader::setScroll(int value)
{
    verticalScroll = value;
    update();
}

void ShiftGraphNameHeader::setZoom(int val)
{
    mZoom = val;
    update();
}

void ShiftGraphNameHeader::paintEvent(QPaintEvent *e)
{
    if(!m_scene)
        return; //Nothing to draw

    QPainter painter(this);
    QColor c(255, 255, 255, 220);
    painter.fillRect(rect(), c);

    const double scaleFactor = mZoom / 100.0;

    const double sceneScroll = verticalScroll / scaleFactor;
    QRectF sceneRect(QPointF(e->rect().topLeft()) / scaleFactor,
                     QSizeF(e->rect().size()) / scaleFactor);

    painter.scale(scaleFactor, scaleFactor);

    m_scene->drawShiftHeader(&painter, sceneRect, sceneScroll);
}
