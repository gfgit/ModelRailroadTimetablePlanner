#include "selectstationpage.h"
#include "../stationimportwizard.h"

#include <QVBoxLayout>
#include "utils/sqldelegate/customcompletionlineedit.h"
#include "utils/sqldelegate/isqlfkmatchmodel.h"

#include <QPushButton>

#include "utils/owningqpointer.h"
#include <QInputDialog>
#include <QMessageBox>
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

    //Reset station
    setStation(0, QString());
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

void SelectStationPage::setStation(db_id stId, const QString &name)
{
    mStationId = stId;
    mStName = name;

    stationLineEdit->setData(mStationId, mStName);

    openDlgBut->setEnabled(mStationId != 0);
    openSvgBut->setEnabled(mStationId != 0);
    importBut->setEnabled(mStationId != 0);
}

void SelectStationPage::onCompletionDone()
{
    db_id stId = 0;
    QString name;
    stationLineEdit->getData(stId, name);

    setStation(stId, name);
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

    //Allow changing name
    OwningQPointer<QInputDialog> dlg = new QInputDialog(this);
    dlg->setWindowTitle(tr("Add Station"));
    dlg->setLabelText(tr("Please choose a name for the new station."));
    dlg->setTextValue(mStName); //Initial value to original name

    do{
        int ret = dlg->exec();
        if(ret != QDialog::Accepted || !dlg)
        {
            break; //User canceled
        }

        const QString name = dlg->textValue().simplified();
        if(name.isEmpty())
        {
            QMessageBox::warning(this, tr("Error"), tr("Station name cannot be empty."));
            continue; //Second chance
        }

        if(mWizard->addStation(mStationId, name))
        {
            break; //Done!
        }
    }
    while (true);

    //Reset station
    setStation(0, QString());
}
