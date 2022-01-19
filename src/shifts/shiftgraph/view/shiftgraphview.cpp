#include "shiftgraphview.h"

#include "shifts/shiftgraph/model/shiftgraphscene.h"

#include "shiftgraphnameheader.h"
#include "shiftgraphhourpanel.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include "utils/owningqpointer.h"
#include "shifts/shiftgraph/jobchangeshiftdlg.h"

#include <QPainter>
#include <QPaintEvent>

#include <QScrollBar>

#include <QHelpEvent>
#include <QToolTip>

#include <QMenu>

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
    connect(&AppSettings, &MRTPSettings::shiftGraphOptionsChanged, this, &ShiftGraphView::resizeHeaders);

    viewport()->setContextMenuPolicy(Qt::DefaultContextMenu);

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

QPointF ShiftGraphView::mapToScene(const QPointF &pos, bool *ok)
{
    QPointF scenePos = pos;

    const QPoint origin(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
    const QRect vp = viewport()->rect();

    //Map to viewport
    scenePos -= vp.topLeft();
    if(scenePos.x() < 0 || scenePos.y() < 0)
    {
        if(ok) *ok = false;
        return scenePos;
    }

    //Map to scene
    scenePos -= origin;
    scenePos /= mZoom / 100.0;

    if(ok) *ok = true;
    return scenePos;
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
    if(m_scene && e->type() == QEvent::ToolTip)
    {
        QHelpEvent *ev = static_cast<QHelpEvent *>(e);

        bool ok = false;
        QPointF scenePos = mapToScene(ev->pos(), &ok);
        if(!ok)
            return false;

        QString msg = m_scene->getTooltipAt(scenePos);

        if(msg.isEmpty())
            QToolTip::hideText();
        else
            QToolTip::showText(ev->globalPos(), msg, viewport());

        return true;
    }
    else if(m_scene && e->type() == QEvent::ContextMenu)
    {
        QContextMenuEvent *ev = static_cast<QContextMenuEvent *>(e);

        bool ok = false;
        QPointF scenePos = mapToScene(ev->pos(), &ok);
        if(!ok)
            return false;

        JobEntry job = m_scene->getJobAt(scenePos);
        if(!job.jobId)
            return false;

        QMenu *menu = new QMenu(this);
        menu->addAction(tr("Show Job in Graph"), this, [job]()
                        {
                            Session->getViewManager()->requestJobSelection(job.jobId, true, true);
                        });
        menu->addAction(tr("Show in Job Editor"), this, [job]()
                        {
                            Session->getViewManager()->requestJobEditor(job.jobId);
                        });
        menu->addAction(tr("Change Job Shift"), this, [this, job]()
                        {
                            OwningQPointer<JobChangeShiftDlg> dlg = new JobChangeShiftDlg(Session->m_Db, this);
                            dlg->setJob(job.jobId);
                            dlg->exec();
                        });

        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(ev->globalPos());

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

    m_scene->drawHourLines(&painter, sceneRect);
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
