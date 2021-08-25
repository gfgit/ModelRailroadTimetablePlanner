#include "backgroundhelper.h"

#include "app/session.h"

#include  "graph/model/linegraphscene.h"

#include <QPainter>

#include <QtMath>

void BackgroundHelper::drawHourPanel(QPainter *painter, const QRectF& rect, int verticalScroll)
{
    //TODO: settings
    QFont hourTextFont;
    QPen hourTextPen(AppSettings.getHourTextColor());

    const int vertOffset = Session->vertOffset;
    const int hourOffset = Session->hourOffset;

    painter->setFont(hourTextFont);
    painter->setPen(hourTextPen);

    //qDebug() << "Drawing hours..." << rect << scroll;
    const QString fmt(QStringLiteral("%1:00"));

    const qreal top = verticalScroll;
    const qreal bottom = rect.bottom();

    int h = qFloor(top / hourOffset);
    qreal y = h * hourOffset - verticalScroll + vertOffset;

    for(; h <= 24 && y <= bottom; h++)
    {
        //qDebug() << "Y:" << y << fmt.arg(h);
        painter->drawText(QPointF(5, y + 8), fmt.arg(h)); //y + 8 to center text vertically
        y += hourOffset;
    }
}

void BackgroundHelper::drawBackgroundHourLines(QPainter *painter, const QRectF &rect)
{
    const double vertOffset = Session->vertOffset;
    const double hourOffset = Session->hourOffset;
    const double hourHorizOffset = AppSettings.getHourLineOffset();

    QPen hourLinePen(AppSettings.getHourLineColor(), AppSettings.getHourLineWidth());

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

void BackgroundHelper::drawStationHeader(QPainter *painter, LineGraphScene *scene, const QRectF &rect, int horizontalScroll)
{
    QFont stationFont;
    stationFont.setBold(true);
    stationFont.setPointSize(12);

    QPen stationPen(AppSettings.getStationTextColor());

    QFont platfBoldFont = stationFont;
    platfBoldFont.setPointSize(10);

    QFont platfNormalFont = platfBoldFont;
    platfNormalFont.setBold(false);

    QPen electricPlatfPen(Qt::blue);
    QPen nonElectricPlatfPen(Qt::black);

    const qreal platformOffset = Session->platformOffset;
    const int stationOffset = Session->stationOffset;

    //On left go back by half station offset to center station label
    //and center platform label by going a back of half platformOffset
    const int leftOffset = -stationOffset/2 - platformOffset /2;

    const double margin = stationOffset * 0.1;

    QRectF r = rect;

    for(auto st : qAsConst(scene->stations))
    {
        const double left = st.xPos + leftOffset - horizontalScroll;
        const double right = left + st.platforms.count() * platformOffset + stationOffset;

        if(right <= 0 || left >= r.width())
            continue; //Skip station, it's not visible

        QRectF labelRect = r;
        labelRect.setLeft(left + margin);
        labelRect.setRight(right - margin);
        labelRect.setBottom(r.bottom() - r.height() / 3);

        painter->setPen(stationPen);
        painter->setFont(stationFont);
        painter->drawText(labelRect, Qt::AlignVCenter | Qt::AlignCenter, st.stationName);

        labelRect = r;
        labelRect.setTop(r.top() + r.height() * 2/3);

        //Go to start of station (first platform)
        //We need to compensate the half stationOffset used to center station label
        double xPos = left + stationOffset/2;
        labelRect.setWidth(platformOffset);
        for(const StationGraphObject::PlatformGraph& platf : qAsConst(st.platforms))
        {
            if(platf.platformType.testFlag(utils::StationTrackType::Electrified))
                painter->setPen(electricPlatfPen);
            else
                painter->setPen(nonElectricPlatfPen);

            if(platf.platformType.testFlag(utils::StationTrackType::Through))
                painter->setFont(platfBoldFont);
            else
                painter->setFont(platfNormalFont);

            labelRect.moveLeft(xPos);

            painter->drawText(labelRect, Qt::AlignCenter, platf.platformName);

            xPos += platformOffset;
        }
    }
}

void BackgroundHelper::drawStations(QPainter *painter, LineGraphScene *scene, const QRectF &rect)
{
    const QRgb white = qRgb(255, 255, 255);

    //const int horizOffset = Session->horizOffset;
    const int vertOffset = Session->vertOffset;
    //const int stationOffset = Session->stationOffset;
    const double platfOffset = Session->platformOffset;
    const int lastY = vertOffset + Session->hourOffset * 24 + 10;

    const int width = AppSettings.getPlatformLineWidth();
    const QColor mainPlatfColor = AppSettings.getMainPlatfColor();

    QPen platfPen (mainPlatfColor,  width);

    QPointF top(0, vertOffset);
    QPointF bottom(0, lastY);

    for(const StationGraphObject &st : qAsConst(scene->stations))
    {
        const double left = st.xPos;
        const double right = left + st.platforms.count() * platfOffset;

        if(left > rect.right() || right < rect.left())
            continue; //Skip station, it's not visible

        top.rx() = bottom.rx() = st.xPos;

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

void BackgroundHelper::drawJobStops(QPainter *painter, LineGraphScene *scene, const QRectF &rect)
{
    const double platfOffset = Session->platformOffset;

    QPen jobPen;
    jobPen.setWidth(AppSettings.getJobLineWidth());

    QPointF top;
    QPointF bottom;

    for(const StationGraphObject &st : qAsConst(scene->stations))
    {
        const double left = st.xPos;
        const double right = left + st.platforms.count() * platfOffset;

        if(left > rect.right() || right < rect.left())
            continue; //Skip station, it's not visible

        top.rx() = bottom.rx() = st.xPos;

        for(const StationGraphObject::PlatformGraph& platf : st.platforms)
        {
            for(const StationGraphObject::JobStopGraph& jobStop : platf.jobStops)
            {
                if(jobStop.arrivalY > rect.bottom() || jobStop.departureY < rect.top())
                    continue; //Skip, job not visible

                jobPen.setColor(Session->colorForCat(jobStop.category));
                painter->setPen(jobPen);

                top.setY(jobStop.arrivalY);
                bottom.setY(jobStop.departureY);

                painter->drawLine(top, bottom);
            }

            top.rx() += platfOffset;
            bottom.rx() += platfOffset;
        }
    }
}
