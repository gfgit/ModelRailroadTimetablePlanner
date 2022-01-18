#include "shifthourpanel.h"

#include "shiftgraphscene.h"

#include <QPainter>

ShiftGraphHourPanel::ShiftGraphHourPanel(QWidget *parent) :
    QWidget(parent),
    m_scene(nullptr),
    horizontalScroll(0),
    mZoom(100)
{

}

void ShiftGraphHourPanel::setScene(ShiftGraphScene *newScene)
{
    m_scene = newScene;
}

void ShiftGraphHourPanel::setScroll(int value)
{
    horizontalScroll = value;
    update();
}

void ShiftGraphHourPanel::setZoom(int val)
{
    mZoom = val;
    update();
}

void ShiftGraphHourPanel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QColor c(255, 255, 255, 220);
    painter.fillRect(rect(), c);

    const double scaleFactor = mZoom / 100.0;

    const double sceneScroll = horizontalScroll / scaleFactor;
    QRectF sceneRect = rect();
    sceneRect.setSize(sceneRect.size() / scaleFactor);

    painter.scale(scaleFactor, scaleFactor);

    if(m_scene)
        m_scene->drawHourHeader(&painter, sceneRect, sceneScroll);
}
