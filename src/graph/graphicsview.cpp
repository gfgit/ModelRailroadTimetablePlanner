#include "graphicsview.h"

#include "graphmanager.h"
#include "backgroundhelper.h"

#include "hourpane.h"
#include "stationlayer.h"

#include <QScrollBar>

#include <QDebug>

#include <QResizeEvent>
#include <QMoveEvent>

GraphicsView::GraphicsView(GraphManager *mgr, QWidget *parent) :
    QGraphicsView(parent),
    helper(mgr->getBackGround())
{
    //setRenderHint(QPainter::Antialiasing); It blurs background lines with big pen sizes
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    hourPane = new HourPane(helper, this);
    stationLayer = new StationLayer(mgr, this);
    updateHourHorizOffset();
    updateStationVertOffset();

    connect(verticalScrollBar(), &QScrollBar::valueChanged, hourPane, &HourPane::setScroll);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, stationLayer, &StationLayer::setScroll);

    connect(helper, &BackgroundHelper::horizHorizOffsetChanged, this, &GraphicsView::updateHourHorizOffset);
    connect(helper, &BackgroundHelper::vertOffsetChanged, this, &GraphicsView::updateStationVertOffset);
    connect(helper, &BackgroundHelper::updateGraph, this, static_cast<void(QGraphicsView::*)()>(&GraphicsView::update));
}

void GraphicsView::drawBackground(QPainter *painter, const QRectF &rect)
{
    helper->drawBackgroundLines(painter, rect);
}

void GraphicsView::updateHourHorizOffset()
{
    hourPane->resize(int(helper->getHourHorizOffset()) - 5, viewport()->height());
}

void GraphicsView::updateStationVertOffset()
{
    stationLayer->resize(viewport()->width(), int(helper->getVertOffset()) - 5);
}

void GraphicsView::redrawStationNames()
{
    stationLayer->update();
}

bool GraphicsView::viewportEvent(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::Resize:
    {
        //qDebug() << e << "Resizing HourPane";
        //qDebug() << "View:" << rect() << "Viewport:" << viewport()->rect();
        hourPane->resize(hourPane->width(), viewport()->height());
        hourPane->setScroll(verticalScrollBar()->value());

        stationLayer->resize(viewport()->width(), stationLayer->height());
        stationLayer->setScroll(horizontalScrollBar()->value());

        break;
    }
    default:
        break;
    }

    return QGraphicsView::viewportEvent(e);
}
