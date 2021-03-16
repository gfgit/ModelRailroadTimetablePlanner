#include "stationlayer.h"

#include <QPainter>

#include "graphmanager.h"
#include "backgroundhelper.h"

#include "app/session.h"
#include "lines/linestorage.h"

StationLayer::StationLayer(GraphManager *mgr, QWidget *parent) :
    QWidget(parent),
    graphMgr(mgr),
    horizontalScroll(0)
{
    connect(Session->mLineStorage, &LineStorage::stationNameChanged,
            this, static_cast<void(QWidget::*)()>(&StationLayer::update));
}

void StationLayer::setScroll(int value)
{
    horizontalScroll = value;
    update();
}

void StationLayer::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QColor c(255, 255, 255, 220);
    p.fillRect(rect(), c);

    db_id lineId = graphMgr->getCurLineId();
    if(lineId == 0)
        return; //No line selected

    graphMgr->getBackGround()->drawForegroundStationLabels(&p, rect(),
                                                           horizontalScroll,
                                                           lineId);
}
