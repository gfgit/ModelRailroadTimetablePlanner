#include "hourpanel.h"

#include "app/session.h"

#include <QPainter>
#include <QtMath>

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

    //TODO: settings
    QFont hourTextFont;
    QPen hourTextPen(AppSettings.getHourTextColor());

    const int vertOffset = Session->vertOffset;
    const int hourOffset = Session->hourOffset;

    painter.setFont(hourTextFont);
    painter.setPen(hourTextPen);

    //qDebug() << "Drawing hours..." << rect << scroll;
    const QString fmt(QStringLiteral("%1:00"));

    const qreal top = verticalScroll;
    const qreal bottom = rect().bottom();

    int h = qFloor(top / hourOffset);
    qreal y = h * hourOffset - verticalScroll + vertOffset;

    for(; h <= 24 && y <= bottom; h++)
    {
        //qDebug() << "Y:" << y << fmt.arg(h);
        painter.drawText(QPointF(5, y + 8), fmt.arg(h)); //y + 8 to center text vertically
        y += hourOffset;
    }
}
