#include "stationsvgplandlg.h"

#include <QIODevice>
#include <QXmlStreamReader>
#include <QSvgRenderer>

#include <QVBoxLayout>

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
    lay->addWidget(view);
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
