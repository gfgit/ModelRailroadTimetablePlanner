#include "backgroundhelper.h"

#include "app/scopedebug.h"

#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>

#include <QPainter>

#include <QtMath>

#include "app/session.h"

BackgroundHelper::BackgroundHelper(QObject *parent) :
    QObject(parent)
{

}

BackgroundHelper::~BackgroundHelper()
{

}

void BackgroundHelper::setHourLinePen(const QPen& pen)
{
    if(hourLinePen == pen)
        return;

    hourLinePen = pen;
    emit updateGraph();
}

void BackgroundHelper::setHourTextPen(const QPen &pen)
{
    if(hourTextPen == pen)
        return;

    hourTextPen = pen;
    emit updateGraph();
}

void BackgroundHelper::setHourTextFont(const QFont& font)
{
    if(hourTextFont == font)
        return;

    hourTextFont = font;
    emit updateGraph();
}

void BackgroundHelper::setHourOffset(qreal value)
{
    hourOffset = value;
    emit updateGraph();
}

void BackgroundHelper::setVertOffset(qreal value)
{
    vertOffset = value;
    emit vertOffsetChanged();
    emit updateGraph();
}

qreal BackgroundHelper::getVertOffset() const
{
    return vertOffset;
}

void BackgroundHelper::setHourHorizOffset(qreal value)
{
    hourHorizOffset = value;
    emit horizHorizOffsetChanged();
    emit updateGraph();
}

qreal BackgroundHelper::getHourHorizOffset() const
{
    return hourHorizOffset;
}

void BackgroundHelper::drawBackgroundLines(QPainter *painter, const QRectF &rect)
{
    const qreal x1 = qMax(qreal(hourHorizOffset), rect.left());
    const qreal x2 = rect.right();
    const qreal t = qMax(rect.top(), vertOffset);
    const qreal b = rect.bottom();

    if(x1 > x2 || b < vertOffset || t > b)
        return;

    qreal f = std::remainder(t - vertOffset, hourOffset);

    if(f < 0)
        f += hourOffset;
    qreal f1 = qFuzzyIsNull(f) ? vertOffset : qMax(t - f + hourOffset, vertOffset);


    const qreal l = std::remainder(b - vertOffset, hourOffset);
    const qreal l1 = b - l;

    std::size_t n = std::size_t((l1 - f1)/hourOffset) + 1;

    QLineF *arr = new QLineF[n];
    for(std::size_t i = 0; i < n; i++)
    {
        arr[i] = QLineF(x1, f1, x2, f1);
        f1 += hourOffset;
    }

    painter->setPen(hourLinePen);
    painter->drawLines(arr, int(n));
    delete [] arr;
}

void BackgroundHelper::drawForegroundHours(QPainter *painter, const QRectF &rect, int scroll)
{
    painter->setFont(hourTextFont);
    painter->setPen(hourTextPen);

    //qDebug() << "Drawing hours..." << rect << scroll;
    const QString fmt(QStringLiteral("%1:00"));

    const qreal top = scroll;
    const qreal bottom = rect.bottom();

    int h = qFloor(top / hourOffset);
    qreal y = h * hourOffset - scroll + vertOffset;

    for(; h <= 24 && y <= bottom; h++)
    {
        //qDebug() << "Y:" << y << fmt.arg(h);
        painter->drawText(QPointF(5, y + 8), fmt.arg(h)); //y + 8 to center text vertically
        y += hourOffset;
    }
}

void BackgroundHelper::drawForegroundStationLabels(QPainter *painter, const QRectF &rect, int hScroll, db_id lineId)
{
    query q(Session->m_Db, "SELECT s.name,s.short_name,s.platforms,s.depot_platf FROM railways"
                           " JOIN stations s ON s.id=railways.stationId"
                           " WHERE railways.lineId=? ORDER BY railways.pos_meters ASC");
    q.bind(1, lineId);

    QFont f;
    f.setBold(true);
    f.setPointSize(15);
    painter->setFont(f);
    painter->setPen(AppSettings.getStationTextColor());

    const qreal platformOffset = Session->platformOffset;
    const int stationOffset = Session->stationOffset;

    qreal x = Session->horizOffset;

    QRectF r = rect;

    for(auto station : q)
    {
        QString stName;
        if(station.column_bytes(1) == 0)
            stName = station.get<QString>(0); //Fallback to full name
        else
            stName = station.get<QString>(1);

        int platf = station.get<int>(2);
        platf += station.get<int>(3);

        r.setLeft(x - hScroll); //Eat width
        painter->drawText(r, Qt::AlignVCenter, stName);

        x += stationOffset + platf * platformOffset;
    }
}
