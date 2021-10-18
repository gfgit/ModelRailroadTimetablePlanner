#include "stationsvgplandlg.h"

#include <QIODevice>
#include <QXmlStreamReader>
#include <QSvgRenderer>

#include <QVBoxLayout>
#include <QScrollArea>
#include <QToolBar>

#include <QSpinBox>

#include <ssplib/svgstationplanlib.h>

StationSVGPlanDlg::StationSVGPlanDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    mDb(db),
    stationId(0)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    m_plan = new ssplib::StationPlan;
    mSvg = new QSvgRenderer(this);

    view = new ssplib::SSPViewer(m_plan);
    view->setRenderer(mSvg);

    toolBar = new QToolBar;
    lay->addWidget(toolBar);

    scrollArea = new QScrollArea(this);
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setWidget(view);
    lay->addWidget(scrollArea);

    QSpinBox *zoomSpin = new QSpinBox;
    zoomSpin->setRange(25, 400);
    connect(zoomSpin, qOverload<int>(&QSpinBox::valueChanged), this, &StationSVGPlanDlg::setZoom);
    connect(this, &StationSVGPlanDlg::zoomChanged, zoomSpin, &QSpinBox::setValue);

    QAction *zoomAction = toolBar->addWidget(zoomSpin);
    zoomAction->setText(tr("Zoom"));

    toolBar->addAction(tr("Fit To Window"), this, &StationSVGPlanDlg::zoomToFit);
}

StationSVGPlanDlg::~StationSVGPlanDlg()
{
    view->setPlan(nullptr);
    view->setRenderer(nullptr);

    delete m_plan;
    m_plan = nullptr;
}

void StationSVGPlanDlg::setStation(db_id stId)
{
    stationId = stId;
}

void StationSVGPlanDlg::reloadSVG(QIODevice *dev)
{
    m_plan->clear();

    //TODO: load station data from DB

    ssplib::StreamParser parser(m_plan, dev);
    parser.parse();

    dev->reset();

    QXmlStreamReader xml(dev);
    mSvg->load(&xml);

    view->update();
}

void StationSVGPlanDlg::setZoom(int val)
{
    if(val == m_zoom || val > 400 || val < 25)
        return;

    m_zoom = val;
    emit zoomChanged(m_zoom);

    QSize s = scrollArea->widget()->sizeHint();
    s = s * m_zoom / 100;
    scrollArea->widget()->resize(s);
}

void StationSVGPlanDlg::zoomToFit()
{
    const QSize available = scrollArea->size();
    const QSize contents = scrollArea->widget()->sizeHint();

    const int zoomH = 100 * available.width() / contents.width();
    const int zoomV = 100 * available.height() / contents.height();

    const int val = qMin(zoomH, zoomV);
    setZoom(val);
}
