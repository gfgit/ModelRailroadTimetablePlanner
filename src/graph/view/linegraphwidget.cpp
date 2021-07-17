#include "linegraphwidget.h"

#include "graph/model/linegraphscene.h"

#include "linegraphviewport.h"
#include "stationlabelsheader.h"

#include "hourpanel.h"

#include "app/session.h"

#include <QBoxLayout>

#include <QScrollArea>
#include <QScrollBar>

LineGraphWidget::LineGraphWidget(QWidget *parent) :
    QWidget(parent)
{
    QHBoxLayout *lay = new QHBoxLayout(this);

    scrollArea = new QScrollArea(this);
    scrollArea->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    lay->addWidget(scrollArea);

    viewport = new LineGraphViewport;
    scrollArea->setWidget(viewport);

    hourPanel = new HourPanel(scrollArea);
    stationHeader = new StationLabelsHeader(scrollArea);

    m_scene = new LineGraphScene(Session->m_Db, this);
    viewport->setScene(m_scene);
    stationHeader->setScene(m_scene);

    connect(scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, hourPanel, &HourPanel::setScroll);
    connect(scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, stationHeader, &StationLabelsHeader::setScroll);
    connect(&AppSettings, &TrainTimetableSettings::jobGraphOptionsChanged, this, &LineGraphWidget::resizeHeaders);

    resizeHeaders();
}

void LineGraphWidget::resizeHeaders()
{
    const int horizOffset = Session->horizOffset;
    const int vertOffset = Session->vertOffset;

    QWidget *wp = scrollArea->viewport();

    hourPanel->resize(horizOffset - 5, wp->height());
    hourPanel->setScroll(scrollArea->verticalScrollBar()->value());

    stationHeader->resize(wp->width(), vertOffset - 5);
    stationHeader->setScroll(scrollArea->horizontalScrollBar()->value());
}
