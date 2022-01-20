#include "basicgraphview.h"

#include "igraphscene.h"
#include "basicgraphheader.h"

#include <QScrollBar>

#include <QPainter>
#include <QPaintEvent>

BasicGraphView::BasicGraphView(QWidget *parent) :
    QAbstractScrollArea(parent),
    m_verticalHeader(nullptr),
    m_horizontalHeader(nullptr),
    m_scene(nullptr),
    mZoom(100)
{
    QPalette pal = palette();
    pal.setColor(backgroundRole(), Qt::white);
    setPalette(pal);

    horizontalScrollBar()->setSingleStep(20);
    verticalScrollBar()->setSingleStep(20);

    m_verticalHeader = new BasicGraphHeader(Qt::Vertical, this);
    m_horizontalHeader = new BasicGraphHeader(Qt::Horizontal, this);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, m_verticalHeader, &BasicGraphHeader::setScroll);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, m_horizontalHeader, &BasicGraphHeader::setScroll);

    resizeHeaders();
}

IGraphScene *BasicGraphView::scene() const
{
    return m_scene;
}

void BasicGraphView::setScene(IGraphScene *newScene)
{
    m_verticalHeader->setScene(newScene);
    m_horizontalHeader->setScene(newScene);

    if(m_scene)
    {
        disconnect(m_scene, &IGraphScene::redrawGraph, this, &BasicGraphView::redrawGraph);
        disconnect(m_scene, &IGraphScene::headersSizeChanged, this, &BasicGraphView::resizeHeaders);
        disconnect(m_scene, &IGraphScene::requestShowRect, this, &BasicGraphView::ensureRectVisible);
        disconnect(m_scene, &QObject::destroyed, this, &BasicGraphView::onSceneDestroyed);
    }
    m_scene = newScene;
    if(m_scene)
    {
        connect(m_scene, &IGraphScene::redrawGraph, this, &BasicGraphView::redrawGraph);
        connect(m_scene, &IGraphScene::headersSizeChanged, this, &BasicGraphView::resizeHeaders);
        connect(m_scene, &IGraphScene::requestShowRect, this, &BasicGraphView::ensureRectVisible);
        connect(m_scene, &QObject::destroyed, this, &BasicGraphView::onSceneDestroyed);
    }

    resizeHeaders();
    redrawGraph();
}

void BasicGraphView::ensureVisible(double x, double y, double xmargin, double ymargin)
{
    if(!m_scene)
        return;

    auto hbar = horizontalScrollBar();
    auto vbar = verticalScrollBar();
    auto vp = viewport();

    const double scaleFactor = mZoom / 100.0;

    //Prevent going under headers
    QSizeF headerSize = m_scene->getHeaderSize().toSize();

    //Apply scaling
    headerSize *= scaleFactor;

    //const double horizOffset = (hourPanel->width() + 5) * scaleFactor;
    //const double vertOffset = (stationHeader->height() + 5) * scaleFactor;

    x *= scaleFactor;
    y *= scaleFactor;
    xmargin *= scaleFactor;
    ymargin *= scaleFactor;

    int logicalX = x;

    double val = hbar->value();
    if (logicalX - xmargin < hbar->value() + headerSize.width())
    {
        val = qMax(0.0, logicalX - xmargin - headerSize.width());
    }
    else if (logicalX > hbar->value() + vp->width() - xmargin)
    {
        val = qMin(logicalX - vp->width() + xmargin, double(hbar->maximum()));
    }
    hbar->setValue(qRound(val));

    val = vbar->value();
    if (y - ymargin < vbar->value() + headerSize.height())
    {
        val = qMax(0.0, y - ymargin - headerSize.height());
    }
    else if (y > vbar->value() + vp->height() - ymargin)
    {
        val = qMin(y - vp->height() + ymargin, double(vbar->maximum()));
    }
    vbar->setValue(qRound(val));
}

QPointF BasicGraphView::mapToScene(const QPointF &pos) const
{
    QPointF scenePos = pos;

    const QPoint origin(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
    const QRect vp = viewport()->rect();

    //Map to viewport
    scenePos -= vp.topLeft();

    //Map to scene
    scenePos -= origin;
    scenePos /= mZoom / 100.0;

    return scenePos;
}

void BasicGraphView::redrawGraph()
{
    updateScrollBars();
    viewport()->update();
    m_verticalHeader->update();
    m_horizontalHeader->update();
}

void BasicGraphView::ensureRectVisible(const QRectF &r)
{
    //FIXME: better implementation
    QRect r2 = r.toRect();
    const QPoint c = r2.center();
    ensureVisible(c.x(), c.y(), r2.width() / 2, r2.height() / 2);
}

void BasicGraphView::setZoomLevel(int zoom)
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
    m_verticalHeader->setZoom(mZoom);
    m_horizontalHeader->setZoom(mZoom);

    emit zoomLevelChanged(mZoom);

    updateScrollBars();
    resizeHeaders();

    //Try set values
    hbar->setValue(horizScroll);
    vbar->setValue(vertScroll);

    //Headers get already updated by setting zoom
    //Now update maint contents
    viewport()->update();
}

bool BasicGraphView::event(QEvent *e)
{
    if (e->type() == QEvent::StyleChange || e->type() == QEvent::LayoutRequest)
    {
        updateScrollBars();
        resizeHeaders();
    }

    return QAbstractScrollArea::event(e);
}

void BasicGraphView::paintEvent(QPaintEvent *e)
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

    m_scene->renderContents(&painter, sceneRect);
}

void BasicGraphView::resizeEvent(QResizeEvent *)
{
    updateScrollBars();
    resizeHeaders();
}

void BasicGraphView::focusInEvent(QFocusEvent *e)
{
    if(m_scene)
        m_scene->activateScene();

    QAbstractScrollArea::focusInEvent(e);
}

void BasicGraphView::onSceneDestroyed()
{
    m_scene = nullptr;
    m_verticalHeader->setScene(nullptr);
    m_horizontalHeader->setScene(nullptr);
    redrawGraph();
}

void BasicGraphView::resizeHeaders()
{
    QSizeF headerSize;
    if(m_scene)
        headerSize = m_scene->getHeaderSize().toSize();

    //Apply scaling
    headerSize *= mZoom / 100.0;

    const QRect vg = viewport()->geometry();

    m_verticalHeader->move(vg.topLeft());
    m_verticalHeader->resize(qRound(headerSize.width()), vg.height());
    m_verticalHeader->setScroll(verticalScrollBar()->value());

    m_horizontalHeader->move(vg.topLeft());
    m_horizontalHeader->resize(vg.width(), qRound(headerSize.height()));
    m_horizontalHeader->setScroll(horizontalScrollBar()->value());
}

void BasicGraphView::updateScrollBars()
{
    if(!m_scene)
        return;

    QSizeF contentsSize = m_scene->getContentsSize();
    if(contentsSize.isEmpty())
        return;

    //Scale contents
    contentsSize *= mZoom / 100.0;

    QSize roundedContentsSize = contentsSize.toSize();

    QSize p = viewport()->size();
    QSize m = maximumViewportSize();

    if(m.expandedTo(roundedContentsSize) == m)
        p = m; //No scrollbars needed

    auto hbar = horizontalScrollBar();
    hbar->setRange(0, roundedContentsSize.width() - p.width());
    hbar->setPageStep(p.width());

    auto vbar = verticalScrollBar();
    vbar->setRange(0, roundedContentsSize.height() - p.height());
    vbar->setPageStep(p.height());
}
