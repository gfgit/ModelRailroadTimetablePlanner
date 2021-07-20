#include "hourpanel.h"

#include "app/session.h"

#include "backgroundhelper.h"

#include <QPainter>

HourPanel::HourPanel(QWidget *parent) :
    QWidget(parent),
    verticalScroll(0)
{
    connect(&AppSettings, &TrainTimetableSettings::jobGraphOptionsChanged, this, qOverload<>(&HourPanel::update));
}

void HourPanel::setScroll(int value)
{
    verticalScroll = value;
    update();
}

void HourPanel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QColor c(255, 255, 255, 220);
    painter.fillRect(rect(), c);

    BackgroundHelper::drawHourPanel(&painter, rect(), verticalScroll);
}
