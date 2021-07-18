#include "linegraphview.h"

#include "graph/model/linegraphscene.h"

#include "stationlabelsheader.h"
#include "hourpanel.h"

#include "app/session.h"

#include <QPainter>
#include <QPaintEvent>

#include <QScrollBar>

LineGraphView::LineGraphView(QWidget *parent) :
    QAbstractScrollArea(parent)
{
    horizontalScrollBar()->setSingleStep(20);
    verticalScrollBar()->setSingleStep(20);

    hourPanel = new HourPanel(this);
    stationHeader = new StationLabelsHeader(this);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, hourPanel, &HourPanel::setScroll);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, stationHeader, &StationLabelsHeader::setScroll);
    connect(&AppSettings, &TrainTimetableSettings::jobGraphOptionsChanged, this, &LineGraphView::resizeHeaders);
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
        disconnect(m_scene, &QObject::destroyed, this, &LineGraphView::onSceneDestroyed);
    }
    m_scene = newScene;
    if(m_scene)
    {
        connect(m_scene, &LineGraphScene::redrawGraph, this, &LineGraphView::redrawGraph);
        connect(m_scene, &QObject::destroyed, this, &LineGraphView::onSceneDestroyed);
    }
    redrawGraph();
}

void LineGraphView::redrawGraph()
{
    recalcContentSize();
    updateScrollBars();

    update();
}

bool LineGraphView::event(QEvent *e)
{
    if (e->type() == QEvent::StyleChange || e->type() == QEvent::LayoutRequest)
    {
        updateScrollBars();
    }

    return QAbstractScrollArea::event(e);
}

void LineGraphView::paintEvent(QPaintEvent *e)
{
    //TODO: repaint only new regions, not all

    QPainter painter(viewport());

    //FIXME: when setting NoGraph it doesn't clear, you have to scroll a bit
    //painter.fillRect(e->rect(), palette().window());
    painter.fillRect(viewport()->rect(), palette().window());

    //Scroll contents
    painter.translate(-horizontalScrollBar()->value(), -verticalScrollBar()->value());

    if(!m_scene || m_scene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to draw

    paintStations(&painter);
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

void LineGraphView::onSceneDestroyed()
{
    m_scene = nullptr;
    redrawGraph();
}

void LineGraphView::resizeHeaders()
{
    const int horizOffset = Session->horizOffset;
    const int vertOffset = Session->vertOffset;

    hourPanel->resize(horizOffset - 5, viewport()->height());
    hourPanel->setScroll(verticalScrollBar()->value());

    stationHeader->resize(viewport()->width(), vertOffset - 5);
    stationHeader->setScroll(horizontalScrollBar()->value());
}

void LineGraphView::recalcContentSize()
{
    contentSize = QSize();

    if(!m_scene || m_scene->getGraphType() == LineGraphType::NoGraph)
        return; //Nothing to draw

    if(m_scene->stationPositions.isEmpty())
        return;

    const auto entry = m_scene->stationPositions.last();
    const int platfCount = m_scene->stations.value(entry.stationId).platforms.count();

    const int maxWidth = Session->horizOffset + entry.xPos + platfCount * Session->platformOffset;
    const int lastY = Session->vertOffset + Session->hourOffset * 24 + 10;

    contentSize = QSize(maxWidth, lastY);
}

void LineGraphView::updateScrollBars()
{
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

    resizeHeaders();
}

void LineGraphView::ensureVisible(int x, int y, int xmargin, int ymargin)
{
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

void LineGraphView::paintStations(QPainter *painter)
{
    const QRgb white = qRgb(255, 255, 255);

    const int horizOffset = Session->horizOffset;
    const int vertOffset = Session->vertOffset;
    const int stationOffset = Session->stationOffset;
    const double platfOffset = Session->platformOffset;
    const int lastY = vertOffset + Session->hourOffset * 24 + 10;

    const int width = AppSettings.getPlatformLineWidth();
    const QColor mainPlatfColor = AppSettings.getMainPlatfColor();

    QPen platfPen (mainPlatfColor,  width);

    QPointF top(0, vertOffset);
    QPointF bottom(0, lastY);

    for(const StationGraphObject &st : qAsConst(m_scene->stations))
    {
        top.rx() = bottom.rx() = st.xPos + horizOffset;

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
