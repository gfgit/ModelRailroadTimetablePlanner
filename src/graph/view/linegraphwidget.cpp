#include "linegraphwidget.h"

#include "graph/model/linegraphscene.h"

#include "linegraphviewport.h"
#include "linegraphtoolbar.h"
#include "linegraphscrollarea.h"

#include "app/session.h"

#include <QVBoxLayout>

LineGraphWidget::LineGraphWidget(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    toolBar = new LineGraphToolbar(this);
    lay->addWidget(toolBar);

    scrollArea = new LineGraphScrollArea(this);
    lay->addWidget(scrollArea);

    viewport = new LineGraphViewport;
    scrollArea->setWidget(viewport);

    m_scene = new LineGraphScene(Session->m_Db, this);
    viewport->setScene(m_scene);
    scrollArea->setScene(m_scene);
    toolBar->setScene(m_scene);
}
