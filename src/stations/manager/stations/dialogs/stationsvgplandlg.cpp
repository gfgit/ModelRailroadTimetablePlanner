#include "stationsvgplandlg.h"

#include <QIODevice>
#include <QXmlStreamReader>
#include <QSvgRenderer>

#include <QVBoxLayout>
#include <QScrollArea>
#include <QToolBar>

#include <QSpinBox>

#include <QMessageBox>

#include <ssplib/svgstationplanlib.h>

#include "stations/manager/stations/model/stationsvghelper.h"

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
    connect(view, &ssplib::SSPViewer::labelClicked, this, &StationSVGPlanDlg::onLabelClicked);

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

    setMinimumSize(400, 300);
    resize(600, 500);
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

    //Sort items
    std::sort(m_plan->labels.begin(), m_plan->labels.end());
    std::sort(m_plan->platforms.begin(), m_plan->platforms.end());
    std::sort(m_plan->trackConnections.begin(), m_plan->trackConnections.end());

    reloadDBData();

    dev->reset();

    QXmlStreamReader xml(dev);
    mSvg->load(&xml);

    view->update();
    zoomToFit();
}

void StationSVGPlanDlg::reloadDBData()
{
    //Clear previous data
    for(ssplib::LabelItem& item : m_plan->labels)
    {
        item.visible = false;
        item.itemId = 0;
        item.labelText.clear();
    }

    for(ssplib::TrackItem& item : m_plan->platforms)
    {
        item.visible = false;
        item.itemId = 0;

        item.jobId = 0;
        item.color = ssplib::whiteRGB;
        item.jobName.clear();

        item.trackName.clear();
    }

    for(ssplib::TrackConnectionItem& item : m_plan->trackConnections)
    {
        item.visible = false;
        item.itemId = 0;

        item.jobId = 0;
        item.color = ssplib::whiteRGB;
        item.jobName.clear();

        item.info.gateId = 0;
        item.info.trackId = 0;
    }

    //Reload from database
    StationSVGHelper::loadStationFromDB(mDb, stationId, m_plan);
}

void StationSVGPlanDlg::reloadPlan()
{
    std::unique_ptr<QIODevice> dev;
    dev.reset(StationSVGHelper::loadImage(mDb, stationId));

    if(!dev)
    {
        QMessageBox::warning(this, tr("Error Loading SVG"),
                             tr("Cannot find SVG data"));
        return;
    }

    if(!dev->open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Error Loading SVG"),
                             tr("Cannot read data: %1").arg(dev->errorString()));
        return;
    }

    reloadSVG(dev.get());
    reloadDBData();
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

void StationSVGPlanDlg::onLabelClicked(qint64 gateId, QChar letter, const QString &text)
{
    QMessageBox::information(this, tr("Gate %1").arg(letter),
                             tr("Station: %2").arg(text));
}

void StationSVGPlanDlg::showEvent(QShowEvent *)
{
    //NOTE: when dialog is created it is hidden so it cannot zoom
    //We load the station and then show the dialog
    //Since dialog is hidden at first it cannot calculate zoom
    //So when the dialog is first shown we trigger zoom again.
    zoomToFit();
}
