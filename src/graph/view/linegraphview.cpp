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
    m_scene(nullptr)
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

        QPoint pos = ev->pos();
        const QRect vp = viewport()->rect();

        //Map to viewport
        pos -= vp.topLeft();
        if(pos.x() < 0 || pos.y() < 0)
            return false;

        //Map to scene
        pos -= origin;

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
    const QPoint origin(-horizontalScrollBar()->value(), -verticalScrollBar()->value());

    QRect exposedRect = e->rect();
    const QRect vp = viewport()->rect();

    //Map to viewport
    exposedRect.moveTopLeft(exposedRect.topLeft() - vp.topLeft());
    exposedRect = exposedRect.intersected(vp);

    //Map to scene
    exposedRect.moveTopLeft(exposedRect.topLeft() - origin);

    QPainter painter(viewport());

    //Scroll contents
    painter.translate(origin);

    BackgroundHelper::drawBackgroundHourLines(&painter, exposedRect);

    if(!m_scene || m_scene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to draw

    BackgroundHelper::drawStations(&painter, m_scene, exposedRect);
    BackgroundHelper::drawJobStops(&painter, m_scene, exposedRect, true);
    BackgroundHelper::drawJobSegments(&painter, m_scene, exposedRect, true);
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

    QPoint pos = e->pos();
    const QRect vp = viewport()->rect();

    //Map to viewport
    pos -= vp.topLeft();
    if(pos.x() < 0 || pos.y() < 0)
        return;

    //Map to scene
    pos -= origin;

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
    const int horizOffset = Session->horizOffset;
    const int vertOffset = Session->vertOffset;

    const QRect vg = viewport()->geometry();

    hourPanel->move(vg.topLeft());
    hourPanel->resize(horizOffset - 5, vg.height());
    hourPanel->setScroll(verticalScrollBar()->value());

    stationHeader->move(vg.topLeft());
    stationHeader->resize(vg.width(), vertOffset - 5);
    stationHeader->setScroll(horizontalScrollBar()->value());
}

void LineGraphView::updateScrollBars()
{
    if(!m_scene)
        return;

    const QSize contentSize = m_scene->getContentSize();
    if(contentSize.isEmpty())
        return;

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

void LineGraphView::ensureVisible(int x, int y, int xmargin, int ymargin)
{
    //FIXME: consider verticalOffset and horizOffset for panels

    auto hbar = horizontalScrollBar();
    auto vbar = verticalScrollBar();
    auto vp = viewport();

    int logicalX = x;
    if (logicalX - xmargin < hbar->value()) {
        hbar->setValue(qMax(0, logicalX - xmargin));
    } else if (logicalX > hbar->value() + vp->width() - xmargin) {
        hbar->setValue(qMin(logicalX - vp->width() + xmargin, hbar->maximum()));
    }
    if (y - ymargin < vbar->value()) {
        vbar->setValue(qMax(0, y - ymargin));
    } else if (y > vbar->value() + vp->height() - ymargin) {
        vbar->setValue(qMin(y - vp->height() + ymargin, vbar->maximum()));
    }
}
