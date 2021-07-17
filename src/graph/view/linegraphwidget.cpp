#include "linegraphwidget.h"

#include "graph/model/linegraphscene.h"

#include "linegraphviewport.h"
#include "linegraphscrollarea.h"

#include "app/session.h"

#include <QBoxLayout>

LineGraphWidget::LineGraphWidget(QWidget *parent) :
    QWidget(parent)
{
    QHBoxLayout *lay = new QHBoxLayout(this);

    scrollArea = new LineGraphScrollArea(this);
    lay->addWidget(scrollArea);

    viewport = new LineGraphViewport;
    scrollArea->setWidget(viewport);

    m_scene = new LineGraphScene(Session->m_Db, this);
    viewport->setScene(m_scene);
    scrollArea->setScene(m_scene);
}
