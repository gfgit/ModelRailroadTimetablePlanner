#include "hourpanel.h"

#include "app/session.h"

#include "backgroundhelper.h"

#include <QPainter>

HourPanel::HourPanel(QWidget *parent) :
    QWidget(parent),
    verticalScroll(0),
    mZoom(100)
{
    connect(&AppSettings, &MRTPSettings::jobGraphOptionsChanged, this, qOverload<>(&HourPanel::update));
}

void HourPanel::setScroll(int value)
{
    verticalScroll = value;
    update();
}

void HourPanel::setZoom(int val)
{
    mZoom = val;
    update();
}

void HourPanel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QColor c(255, 255, 255, 220);
    painter.fillRect(rect(), c);

    const double scaleFactor = mZoom / 100.0;

    const double sceneScroll = verticalScroll / scaleFactor;
    QRectF sceneRect = rect();
    sceneRect.setSize(sceneRect.size() / scaleFactor);

    painter.scale(scaleFactor, scaleFactor);

    BackgroundHelper::drawHourPanel(&painter, sceneRect, sceneScroll);
}
