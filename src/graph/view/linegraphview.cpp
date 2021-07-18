#include "linegraphview.h"

#include "graph/model/linegraphscene.h"

#include "stationlabelsheader.h"
#include "hourpanel.h"

#include "app/session.h"

#include <QPainter>
#include <QPaintEvent>

#include <QScrollBar>

#include <QtMath>

LineGraphView::LineGraphView(QWidget *parent) :
    QAbstractScrollArea(parent)
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
    connect(&AppSettings, &TrainTimetableSettings::jobGraphOptionsChanged, this, &LineGraphView::resizeHeaders);

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
    updateScrollBars();

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

void LineGraphView::paintEvent(QPaintEvent *e)
{
    //TODO: repaint only new regions, not all

    //FIXME: paint hour lines

    QPainter painter(viewport());

    //Scroll contents
    const QPoint origin(-horizontalScrollBar()->value(), -verticalScrollBar()->value());

    painter.translate(origin);

    QRect visibleRect(-origin, viewport()->size());
    drawBackgroundHourLines(&painter, visibleRect);

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

void LineGraphView::drawBackgroundHourLines(QPainter *painter, const QRectF &rect)
{
    const double vertOffset = Session->vertOffset;
    const double hourOffset = Session->hourOffset;
    const double hourHorizOffset = AppSettings.getHourLineOffset();

    QPen hourLinePen(AppSettings.getHourLineColor(), AppSettings.getHourLineWidth());

    const qreal x1 = qMax(qreal(hourHorizOffset), rect.left());
    const qreal x2 = rect.right();
    const qreal t = qMax(rect.top(), vertOffset);
    const qreal b = rect.bottom();


    if(x1 > x2 || b < vertOffset || t > b)
        return;

    qreal f = std::remainder(t - vertOffset, hourOffset);

    if(f < 0)
        f += hourOffset;
    qreal f1 = qFuzzyIsNull(f) ? vertOffset : qMax(t - f + hourOffset, vertOffset);


    const qreal l = std::remainder(b - vertOffset, hourOffset);
    const qreal l1 = b - l;

    std::size_t n = std::size_t((l1 - f1)/hourOffset) + 1;

    QLineF *arr = new QLineF[n];
    for(std::size_t i = 0; i < n; i++)
    {
        arr[i] = QLineF(x1, f1, x2, f1);
        f1 += hourOffset;
    }

    painter->setPen(hourLinePen);
    painter->drawLines(arr, int(n));
    delete [] arr;
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
        //Center start of station on middle of stationOffset
        //This way we leave half of stationOffset on left and labels are centered
        top.rx() = bottom.rx() = st.xPos + stationOffset/2 + horizOffset;

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
