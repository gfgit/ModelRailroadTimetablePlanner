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

    QFont f;
    f.setBold(true);
    f.setPointSize(15);
    painter.setFont(f);
    painter.setPen(AppSettings.getStationTextColor());

    const qreal platformOffset = Session->platformOffset;
    const int stationOffset = Session->stationOffset;
    const int horizOffset = Session->horizOffset;

    QRectF r = rect();

    for(auto st : qAsConst(m_scene->stations))
    {
        double left = st.xPos + horizOffset - horizontalScroll;
        double right = left + st.platforms.count() * platformOffset;

        if(right <= 0 || left >= r.width())
            continue; //Skip station, it's not visible

        QRectF labelRect = r;
        labelRect.setLeft(left);
        labelRect.setRight(right);
        labelRect.setBottom(r.bottom() - r.height() / 3);
        painter.drawText(labelRect, Qt::AlignCenter, st.stationName);

        labelRect = r;
        labelRect.setTop(r.top() + r.height() * 2/3);

        double xPos = left - platformOffset/2;
        labelRect.setWidth(platformOffset);
        for(const StationGraphObject::PlatformGraph& platf : st.platforms)
        {
            if(platf.platformType.testFlag(utils::StationTrackType::Electrified))
                painter.setPen(Qt::blue);
            else
                painter.setPen(Qt::black);

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
