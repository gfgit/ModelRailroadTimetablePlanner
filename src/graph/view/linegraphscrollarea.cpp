#include "linegraphscrollarea.h"

#include "stationlabelsheader.h"

#include "hourpanel.h"

#include "app/session.h"

#include <QScrollBar>

LineGraphScrollArea::LineGraphScrollArea(QWidget *parent) :
    QScrollArea(parent)
{
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    hourPanel = new HourPanel(this);
    stationHeader = new StationLabelsHeader(this);

    connect(verticalScrollBar(), &QScrollBar::valueChanged, hourPanel, &HourPanel::setScroll);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, stationHeader, &StationLabelsHeader::setScroll);
    connect(&AppSettings, &TrainTimetableSettings::jobGraphOptionsChanged, this, &LineGraphScrollArea::resizeHeaders);

    resizeHeaders();
}

void LineGraphScrollArea::setScene(LineGraphScene *scene)
{
    stationHeader->setScene(scene);
}

void LineGraphScrollArea::resizeHeaders()
{
    const int horizOffset = Session->horizOffset;
    const int vertOffset = Session->vertOffset;

    hourPanel->resize(horizOffset - 5, viewport()->height());
    hourPanel->setScroll(verticalScrollBar()->value());

    stationHeader->resize(viewport()->width(), vertOffset - 5);
    stationHeader->setScroll(horizontalScrollBar()->value());
}

bool LineGraphScrollArea::viewportEvent(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::Resize:
    {
        resizeHeaders();
        break;
    }
    default:
        break;
    }

    return QScrollArea::viewportEvent(e);
}

void LineGraphScrollArea::mousePressEvent(QMouseEvent *e)
{
    emit syncToolbarToScene();
}
