#include "linegraphview.h"

#include "graph/model/linegraphscene.h"

#include "backgroundhelper.h"
#include "stationlabelsheader.h"
#include "hourpanel.h"

#include "utils/jobcategorystrings.h"

#include "app/session.h"

#include <QPainter>
#include <QPaintEvent>

#include <QScrollBar>

#include <QToolTip>
#include <QHelpEvent>

LineGraphView::LineGraphView(QWidget *parent) :
    QAbstractScrollArea(parent),
    m_scene(nullptr),
    mZoom(100)
{
    QPalette pal = palette();
    pal.setColor(backgroundRole(), Qt::white);
    setPalette(pal);

    horizontalScrollBar()->setSingleStep(20);
    verticalScrollBar()->setSingleStep(20);

    hourPanel = new HourPanel(this);
    stationHeader = new StationLabelsHeader(this);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, hourPanel, &HourPanel::setScroll);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, stationHeader, &StationLabelsHeader::setScroll);
    connect(&AppSettings, &MRTPSettings::jobGraphOptionsChanged, this, &LineGraphView::resizeHeaders);

    resizeHeaders();
}

LineGraphScene *LineGraphView::scene() const
{
    return m_scene;
}

void LineGraphView::setScene(LineGraphScene *newScene)
{
    stationHeader->setScene(newScene);

    if(m_scene)
    {
        disconnect(m_scene, &LineGraphScene::redrawGraph, this, &LineGraphView::redrawGraph);
        disconnect(m_scene, &LineGraphScene::requestShowRect, this, &LineGraphView::ensureRectVisible);
        disconnect(m_scene, &QObject::destroyed, this, &LineGraphView::onSceneDestroyed);
    }
    m_scene = newScene;
    if(m_scene)
    {
        connect(m_scene, &LineGraphScene::redrawGraph, this, &LineGraphView::redrawGraph);
        connect(m_scene, &LineGraphScene::requestShowRect, this, &LineGraphView::ensureRectVisible);
        connect(m_scene, &QObject::destroyed, this, &LineGraphView::onSceneDestroyed);
    }
    redrawGraph();
}

void LineGraphView::redrawGraph()
{
    updateScrollBars();

    viewport()->update();
}

void LineGraphView::ensureRectVisible(const QRectF &r)
{
    QRect r2 = r.toRect();
    const QPoint c = r2.center();
    ensureVisible(c.x(), c.y(), r2.width() / 2, r2.height() / 2);
}

void LineGraphView::setZoomLevel(int zoom)
{
    if(mZoom == zoom)
        return;

    //Bound values
    zoom = qBound(25, zoom, 400);

    mZoom = zoom;
    hourPanel->setZoom(mZoom);
    stationHeader->setZoom(mZoom);

    emit zoomLevelChanged(mZoom);

    updateScrollBars();
    resizeHeaders();
    viewport()->update();
}

bool LineGraphView::event(QEvent *e)
{
    if (e->type() == QEvent::StyleChange || e->type() == QEvent::LayoutRequest)
    {
        updateScrollBars();
        resizeHeaders();
    }

    return QAbstractScrollArea::event(e);
}

bool LineGraphView::viewportEvent(QEvent *e)
{
    if(e->type() == QEvent::ToolTip && m_scene && m_scene->getGraphType() != LineGraphType::NoGraph)
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

        JobStopEntry job = m_scene->getJobAt(pos, Session->platformOffset / 2);

        if(job.jobId)
        {
            QToolTip::showText(ev->globalPos(),
                               JobCategoryName::jobName(job.jobId, job.category),
                               viewport());
        }else{
            QToolTip::hideText();
        }

        return true;
    }

    return QAbstractScrollArea::viewportEvent(e);
}

void LineGraphView::paintEvent(QPaintEvent *e)
{
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

    BackgroundHelper::drawBackgroundHourLines(&painter, sceneRect);

    if(!m_scene || m_scene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to draw

    BackgroundHelper::drawStations(&painter, m_scene, sceneRect);
    BackgroundHelper::drawJobStops(&painter, m_scene, sceneRect, true);
    BackgroundHelper::drawJobSegments(&painter, m_scene, sceneRect, true);
}

void LineGraphView::resizeEvent(QResizeEvent *)
{
    updateScrollBars();
    resizeHeaders();
}

void LineGraphView::mousePressEvent(QMouseEvent *e)
{
    emit syncToolbarToScene();
    QAbstractScrollArea::mousePressEvent(e);
}

void LineGraphView::mouseDoubleClickEvent(QMouseEvent *e)
{
    if(!m_scene || m_scene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to select

    const QPoint origin(-horizontalScrollBar()->value(), -verticalScrollBar()->value());

    QPointF pos = e->pos();
    const QRect vp = viewport()->rect();

    //Map to viewport
    pos -= vp.topLeft();
    if(pos.x() < 0 || pos.y() < 0)
        return;

    //Map to scene
    pos -= origin;
    pos /= mZoom / 100.0;

    JobStopEntry job = m_scene->getJobAt(pos, Session->platformOffset / 2);
    m_scene->setSelectedJob(job);
}

void LineGraphView::focusInEvent(QFocusEvent *e)
{
    if(m_scene)
        m_scene->activateScene();

    QAbstractScrollArea::focusInEvent(e);
}

void LineGraphView::onSceneDestroyed()
{
    m_scene = nullptr;
    redrawGraph();
}

void LineGraphView::resizeHeaders()
{
    const double horizOffset = (Session->horizOffset - 5.0) * mZoom / 100.0;
    const double vertOffset = (Session->vertOffset - 5.0) * mZoom / 100.0;

    const QRect vg = viewport()->geometry();

    hourPanel->move(vg.topLeft());
    hourPanel->resize(qRound(horizOffset), vg.height());
    hourPanel->setScroll(verticalScrollBar()->value());

    stationHeader->move(vg.topLeft());
    stationHeader->resize(vg.width(), qRound(vertOffset));
    stationHeader->setScroll(horizontalScrollBar()->value());
}

void LineGraphView::updateScrollBars()
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

void LineGraphView::ensureVisible(double x, double y, double xmargin, double ymargin)
{
    auto hbar = horizontalScrollBar();
    auto vbar = verticalScrollBar();
    auto vp = viewport();

    const double scaleFactor = mZoom / 100.0;

    const double horizOffset = (hourPanel->width() + 5) * scaleFactor;
    const double vertOffset = (stationHeader->height() + 5) * scaleFactor;

    x *= scaleFactor;
    y *= scaleFactor;
    xmargin *= scaleFactor;
    ymargin *= scaleFactor;

    int logicalX = x;

    double val = hbar->value();
    if (logicalX - xmargin < hbar->value() + horizOffset)
    {
        val = qMax(0.0, logicalX - xmargin - horizOffset);
    }
    else if (logicalX > hbar->value() + vp->width() - xmargin)
    {
        val = qMin(logicalX - vp->width() + xmargin, double(hbar->maximum()));
    }
    hbar->setValue(qRound(val));

    val = vbar->value();
    if (y - ymargin < vbar->value() + vertOffset)
    {
        val = qMax(0.0, y - ymargin - vertOffset);
    }
    else if (y > vbar->value() + vp->height() - ymargin)
    {
        val = qMin(y - vp->height() + ymargin, double(vbar->maximum()));
    }
    vbar->setValue(qRound(val));
}
