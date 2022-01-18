#include "shiftgraphview.h"

#include "shifts/shiftgraph/model/shiftgraphscene.h"

#include "shiftgraphnameheader.h"
#include "shiftgraphhourpanel.h"

#include "app/session.h"

#include <QPainter>
#include <QPaintEvent>

#include <QScrollBar>

#include <QHelpEvent>
#include <QToolTip>

ShiftGraphView::ShiftGraphView(QWidget *parent) :
    QAbstractScrollArea(parent),
    m_scene(nullptr),
    mZoom(100)
{
    QPalette pal = palette();
    pal.setColor(backgroundRole(), Qt::white);
    setPalette(pal);

    horizontalScrollBar()->setSingleStep(20);
    verticalScrollBar()->setSingleStep(20);

    shiftHeader = new ShiftGraphNameHeader(this);
    hourPanel = new ShiftGraphHourPanel(this);

    connect(horizontalScrollBar(), &QScrollBar::valueChanged, hourPanel, &ShiftGraphHourPanel::setScroll);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, shiftHeader, &ShiftGraphNameHeader::setScroll);
    connect(&AppSettings, &MRTPSettings::jobGraphOptionsChanged, this, &ShiftGraphView::resizeHeaders);

    resizeHeaders();
}

ShiftGraphScene *ShiftGraphView::scene() const
{
    return m_scene;
}

void ShiftGraphView::setScene(ShiftGraphScene *newScene)
{
    shiftHeader->setScene(newScene);
    hourPanel->setScene(newScene);

    if(m_scene)
    {
        disconnect(m_scene, &ShiftGraphScene::redrawGraph, this, &ShiftGraphView::redrawGraph);
        disconnect(m_scene, &QObject::destroyed, this, &ShiftGraphView::onSceneDestroyed);
    }
    m_scene = newScene;
    if(m_scene)
    {
        connect(m_scene, &ShiftGraphScene::redrawGraph, this, &ShiftGraphView::redrawGraph);
        connect(m_scene, &QObject::destroyed, this, &ShiftGraphView::onSceneDestroyed);
    }
    redrawGraph();
}

void ShiftGraphView::redrawGraph()
{
    updateScrollBars();
    viewport()->update();
    shiftHeader->update();
    hourPanel->update();
}

void ShiftGraphView::setZoomLevel(int zoom)
{
    if(mZoom == zoom)
        return;

    auto hbar = horizontalScrollBar();
    auto vbar = verticalScrollBar();
    double horizScroll = hbar->value();
    double vertScroll = vbar->value();

    //Bound values
    zoom = qBound(25, zoom, 400);

    //Reposition scrollbars
    horizScroll *= zoom/double(mZoom);
    vertScroll *= zoom/double(mZoom);

    mZoom = zoom;
    hourPanel->setZoom(mZoom);
    shiftHeader->setZoom(mZoom);

    emit zoomLevelChanged(mZoom);

    updateScrollBars();
    resizeHeaders();

    //Try set values
    hbar->setValue(horizScroll);
    vbar->setValue(vertScroll);

    viewport()->update();
}

bool ShiftGraphView::event(QEvent *e)
{
    if (e->type() == QEvent::StyleChange || e->type() == QEvent::LayoutRequest)
    {
        updateScrollBars();
        resizeHeaders();
    }

    return QAbstractScrollArea::event(e);
}

bool ShiftGraphView::viewportEvent(QEvent *e)
{
    if(e->type() == QEvent::ToolTip && m_scene)
    {
        QHelpEvent *ev = static_cast<QHelpEvent *>(e);

        const QPoint origin(-horizontalScrollBar()->value(), -verticalScrollBar()->value());

        QPointF pos = ev->pos();
        const QRect vp = viewport()->rect();

        //Map to viewport
        pos -= vp.topLeft();
        if(pos.x() < 0 || pos.y() < 0)
            return false;

        //Map to scene
        pos -= origin;
        pos /= mZoom / 100.0;

//        JobStopEntry job = m_scene->getJobAt(pos, Session->platformOffset / 2);

//        if(job.jobId)
//        {
//            QToolTip::showText(ev->globalPos(),
//                               JobCategoryName::jobName(job.jobId, job.category),
//                               viewport());
//        }else{
//            QToolTip::hideText();
//        }

        return true;
    }

    return QAbstractScrollArea::viewportEvent(e);
}

void ShiftGraphView::paintEvent(QPaintEvent *e)
{
    if(!m_scene)
        return; //Nothing to draw

    const double scaleFactor = mZoom / 100.0;
    const QPoint origin(-horizontalScrollBar()->value(), -verticalScrollBar()->value());

    QRectF exposedRect = e->rect();
    const QRect vp = viewport()->rect();

    //Map to viewport
    exposedRect.moveTopLeft(exposedRect.topLeft() - vp.topLeft());
    exposedRect = exposedRect.intersected(vp);

    //Map to scene
    exposedRect.moveTopLeft(exposedRect.topLeft() - origin);

    //Set zoom
    const QRectF sceneRect(exposedRect.topLeft() / scaleFactor, exposedRect.size() / scaleFactor);

    QPainter painter(viewport());

    //Scroll contents
    painter.translate(origin);
    painter.scale(scaleFactor, scaleFactor);

    //BackgroundHelper::drawBackgroundHourLines(&painter, sceneRect);

    m_scene->drawShifts(&painter, sceneRect);
}

void ShiftGraphView::resizeEvent(QResizeEvent *)
{
    updateScrollBars();
    resizeHeaders();
}

void ShiftGraphView::onSceneDestroyed()
{
    m_scene = nullptr;
    shiftHeader->setScene(nullptr);
    hourPanel->setScene(nullptr);
    redrawGraph();
}

void ShiftGraphView::resizeHeaders()
{
    const double horizOffset = (AppSettings.getShiftHorizOffset() - 5.0) * mZoom / 100.0;
    const double vertOffset = (AppSettings.getShiftVertOffset() - 5.0) * mZoom / 100.0;

    const QRect vg = viewport()->geometry();

    shiftHeader->move(vg.topLeft());
    shiftHeader->resize(qRound(horizOffset), vg.height());
    shiftHeader->setScroll(verticalScrollBar()->value());

    hourPanel->move(vg.topLeft());
    hourPanel->resize(vg.width(), qRound(vertOffset));
    hourPanel->setScroll(horizontalScrollBar()->value());
}

void ShiftGraphView::updateScrollBars()
{
    if(!m_scene)
        return;

    QSize contentSize = m_scene->getContentSize();
    if(contentSize.isEmpty())
        return;

    //Scale contents
    contentSize *= mZoom / 100.0;

    QSize p = viewport()->size();
    QSize m = maximumViewportSize();

    if(m.expandedTo(contentSize) == m)
        p = m; //No scrollbars needed

    auto hbar = horizontalScrollBar();
    hbar->setRange(0, contentSize.width() - p.width());
    hbar->setPageStep(p.width());

    auto vbar = verticalScrollBar();
    vbar->setRange(0, contentSize.height() - p.height());
    vbar->setPageStep(p.height());
}
