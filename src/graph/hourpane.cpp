#include "hourpane.h"

#include "backgroundhelper.h"

#include <QPainter>

#include <QDebug>

HourPane::HourPane(BackgroundHelper *h, QWidget *parent) :
    QWidget (parent),
    helper(h),
    verticalScroll(0)
{

}

void HourPane::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QColor c(255, 255, 255, 220);
    p.fillRect(rect(), c);
    helper->drawForegroundHours(&p, rect(), verticalScroll);
}

void HourPane::setScroll(int value)
{
    verticalScroll = value;
    update();
}
