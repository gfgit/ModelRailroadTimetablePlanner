#include "stationlabelsheader.h"

#include "graph/model/linegraphscene.h"

#include "app/session.h"

#include <QPainter>

StationLabelsHeader::StationLabelsHeader(QWidget *parent) :
    QWidget(parent),
    m_scene(nullptr),
    horizontalScroll(0)
{

}

void StationLabelsHeader::setScroll(int value)
{
    horizontalScroll = value;
    update();
}

void StationLabelsHeader::onSceneDestroyed()
{
    m_scene = nullptr;
    update();
}

void StationLabelsHeader::paintEvent(QPaintEvent *)
{
    //TODO: repaint only new regions, not all

    if(!m_scene || m_scene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to draw

    QPainter painter(this);
    QColor c(255, 255, 255, 220);
    painter.fillRect(rect(), c);

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

    QRectF r = rect();

    for(auto st : qAsConst(m_scene->stations))
    {
        const double left = st.xPos + leftOffset - horizontalScroll;
        const double right = left + st.platforms.count() * platformOffset + stationOffset;

        if(right <= 0 || left >= r.width())
            continue; //Skip station, it's not visible

        QRectF labelRect = r;
        labelRect.setLeft(left + margin);
        labelRect.setRight(right - margin);
        labelRect.setBottom(r.bottom() - r.height() / 3);

        painter.setPen(stationPen);
        painter.setFont(stationFont);
        painter.drawText(labelRect, Qt::AlignVCenter | Qt::AlignCenter, st.stationName);

        labelRect = r;
        labelRect.setTop(r.top() + r.height() * 2/3);

        //Go to start of station (first platform)
        //We need to compensate the half stationOffset used to center station label
        double xPos = left + stationOffset/2;
        labelRect.setWidth(platformOffset);
        for(const StationGraphObject::PlatformGraph& platf : qAsConst(st.platforms))
        {
            if(platf.platformType.testFlag(utils::StationTrackType::Electrified))
                painter.setPen(electricPlatfPen);
            else
                painter.setPen(nonElectricPlatfPen);

            if(platf.platformType.testFlag(utils::StationTrackType::Through))
                painter.setFont(platfBoldFont);
            else
                painter.setFont(platfNormalFont);

            labelRect.moveLeft(xPos);

            painter.drawText(labelRect, Qt::AlignCenter, platf.platformName);

            xPos += platformOffset;
        }
    }
}

LineGraphScene *StationLabelsHeader::scene() const
{
    return m_scene;
}

void StationLabelsHeader::setScene(LineGraphScene *newScene)
{
    if(m_scene)
    {
        disconnect(m_scene, &LineGraphScene::redrawGraph, this, qOverload<>(&StationLabelsHeader::update));
        disconnect(m_scene, &QObject::destroyed, this, &StationLabelsHeader::onSceneDestroyed);
    }
    m_scene = newScene;
    if(m_scene)
    {
        connect(m_scene, &LineGraphScene::redrawGraph, this, qOverload<>(&StationLabelsHeader::update));
        connect(m_scene, &QObject::destroyed, this, &StationLabelsHeader::onSceneDestroyed);
    }
    update();
}
