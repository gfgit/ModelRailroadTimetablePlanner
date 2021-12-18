#include "selectstationpage.h"
#include "../stationimportwizard.h"

#include <QVBoxLayout>
#include "utils/sqldelegate/customcompletionlineedit.h"
#include "utils/sqldelegate/isqlfkmatchmodel.h"

#include <QPushButton>

#include "utils/owningqpointer.h"
#include "stations/manager/stations/dialogs/stationeditdialog.h"

#include "stations/manager/stations/model/stationsvghelper.h"
#include "stations/manager/stations/dialogs/stationsvgplandlg.h"

SelectStationPage::SelectStationPage(StationImportWizard *w) :
    QWizardPage(w),
    mWizard(w),
    stationsModel(nullptr),
    mStationId(0)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    stationLineEdit = new CustomCompletionLineEdit(nullptr);
    connect(stationLineEdit, &CustomCompletionLineEdit::completionDone, this, &SelectStationPage::onCompletionDone);
    lay->addWidget(stationLineEdit);

    openDlgBut = new QPushButton(tr("View Station"));
    connect(openDlgBut, &QPushButton::clicked, this, &SelectStationPage::openStationDlg);
    lay->addWidget(openDlgBut);

    openSvgBut = new QPushButton(tr("View SVG"));
    connect(openSvgBut, &QPushButton::clicked, this, &SelectStationPage::openStationSVGPlan);
    lay->addWidget(openSvgBut);

    importBut = new QPushButton(tr("Import Station"));
    connect(importBut, &QPushButton::clicked, this, &SelectStationPage::importSelectedStation);
    lay->addWidget(importBut);
}

void SelectStationPage::setupModel(ISqlFKMatchModel *m)
{
    stationsModel = m;
    stationLineEdit->setModel(m);

    stationLineEdit->setData(0);
    onCompletionDone(); //Trigger UI update
}

void SelectStationPage::finalizeModel()
{
    stationLineEdit->setModel(nullptr);

    if(stationsModel)
    {
        delete stationsModel;
        stationsModel = nullptr;
    }
}

void SelectStationPage::onCompletionDone()
{
    stationLineEdit->getData(mStationId, mStName);

    openDlgBut->setEnabled(mStationId != 0);
    openSvgBut->setEnabled(mStationId != 0);
    importBut->setEnabled(mStationId != 0);
}

void SelectStationPage::openStationDlg()
{
    if(!mStationId)
        return;

    //TODO: avoid loading segments and disable Add Image Button
    OwningQPointer<StationEditDialog> dlg = new StationEditDialog(*mWizard->getTempDB(), this);
    dlg->setStationExternalEditingEnabled(false);
    dlg->setStationInternalEditingEnabled(false);

    dlg->setStation(mStationId);
    dlg->setWindowTitle(tr("Imported %1").arg(mStName));
    dlg->exec();
}

void SelectStationPage::openStationSVGPlan()
{
    if(!mStationId)
        return;

    std::unique_ptr<QIODevice> dev;
    dev.reset(StationSVGHelper::loadImage(*mWizard->getTempDB(), mStationId));
    if(!dev || !dev->open(QIODevice::ReadOnly))
        return;

    OwningQPointer<QDialog> dlg = new QDialog(this);
    QVBoxLayout *lay = new QVBoxLayout(dlg);

    StationSVGPlanDlg *svg = new StationSVGPlanDlg(*mWizard->getTempDB(), this);
    lay->addWidget(svg);

    svg->reloadSVG(dev.get());
    dlg->setWindowTitle(tr("Imported %1 SVG Plan").arg(mStName));

    dlg->resize(300, 200);
    dlg->exec();
}

void SelectStationPage::importSelectedStation()
{
    if(!mStationId)
        return;
    //TODO
}
