#include "selectstationpage.h"
#include "../stationimportwizard.h"

#include <QVBoxLayout>
#include <QToolBar>
#include <QLineEdit>
#include <QTableView>
#include <QHeaderView>

#include "utils/owningqpointer.h"
#include <QInputDialog>
#include <QMessageBox>
#include "stations/manager/stations/dialogs/stationeditdialog.h"

#include "stations/manager/stations/model/stationsvghelper.h"
#include "stations/manager/stations/dialogs/stationsvgplandlg.h"

#include "utils/sqldelegate/modelpageswitcher.h"
#include "stations/importer/model/importstationmodel.h"


SelectStationPage::SelectStationPage(StationImportWizard *w) :
    QWizardPage(w),
    mWizard(w),
    stationsModel(nullptr),
    filterTimerId(0)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    toolBar = new QToolBar;
    lay->addWidget(toolBar);

    view = new QTableView;
    lay->addWidget(view);

    auto ps = new ModelPageSwitcher(false, this);
    lay->addWidget(ps);
    ps->setModel(stationsModel);
    //Custom colun sorting
    //NOTE: leave disconnect() in the old SIGLAL()/SLOT() version in order to work
    QHeaderView *header = view->horizontalHeader();
    disconnect(header, SIGNAL(sectionPressed(int)), view, SLOT(selectColumn(int)));
    disconnect(header, SIGNAL(sectionEntered(int)), view, SLOT(_q_selectColumn(int)));
    connect(header, &QHeaderView::sectionClicked, this, [this, header](int section)
            {
                if(!stationsModel)
                    return;
                stationsModel->setSortingColumn(section);
                header->setSortIndicator(stationsModel->getSortingColumn(), Qt::AscendingOrder);
            });
    header->setSortIndicatorShown(true);

    filterNameEdit = new QLineEdit(nullptr);
    filterNameEdit->setPlaceholderText(tr("Filter..."));
    filterNameEdit->setClearButtonEnabled(true);
    connect(filterNameEdit, &QLineEdit::textEdited, this, &SelectStationPage::startFilterTimer);

    actOpenStDlg = toolBar->addAction(tr("View Info"), this, &SelectStationPage::openStationDlg);
    actOpenSVGPlan = toolBar->addAction(tr("SVG Plan"), this, &SelectStationPage::openStationSVGPlan);
    toolBar->addSeparator();
    actImportSt = toolBar->addAction(tr("Import"), this, &SelectStationPage::importSelectedStation);
    toolBar->addSeparator();
    toolBar->addWidget(filterNameEdit);
}

void SelectStationPage::setupModel(ImportStationModel *m)
{
    stationsModel = m;
    view->setModel(m);

    //Sync sort indicator
    view->horizontalHeader()->setSortIndicator(stationsModel->getSortingColumn(), Qt::AscendingOrder);

    //Refresh model
    filterNameEdit->clear();
    updateFilter();
}

void SelectStationPage::finalizeModel()
{
    stopFilterTimer();

    view->setModel(nullptr);

    if(stationsModel)
    {
        delete stationsModel;
        stationsModel = nullptr;
    }
}

void SelectStationPage::updateFilter()
{
    stopFilterTimer();
    QString nameFilter = filterNameEdit->text();
    stationsModel->filterByName(nameFilter);
}

void SelectStationPage::openStationDlg()
{
    if(!view->selectionModel()->hasSelection())
        return;

    QModelIndex idx = view->currentIndex();
    db_id stationId = stationsModel->getIdAtRow(idx.row());
    if(!stationId)
        return;

    const QString stName = stationsModel->getNameAtRow(idx.row());

    OwningQPointer<StationEditDialog> dlg = new StationEditDialog(*mWizard->getTempDB(), this);
    dlg->setStationExternalEditingEnabled(false);
    dlg->setStationInternalEditingEnabled(false);
    dlg->setGateConnectionsVisible(false);

    dlg->setStation(stationId);
    dlg->setWindowTitle(tr("Imported %1").arg(stName));
    dlg->exec();
}

void SelectStationPage::openStationSVGPlan()
{
    if(!view->selectionModel()->hasSelection())
        return;

    QModelIndex idx = view->currentIndex();
    db_id stationId = stationsModel->getIdAtRow(idx.row());
    if(!stationId)
        return;

    const QString stName = stationsModel->getNameAtRow(idx.row());

    std::unique_ptr<QIODevice> dev;
    dev.reset(StationSVGHelper::loadImage(*mWizard->getTempDB(), stationId));
    if(!dev || !dev->open(QIODevice::ReadOnly))
        return;

    OwningQPointer<QDialog> dlg = new QDialog(this);
    QVBoxLayout *lay = new QVBoxLayout(dlg);

    StationSVGPlanDlg *svg = new StationSVGPlanDlg(*mWizard->getTempDB(), this);
    lay->addWidget(svg);

    svg->reloadSVG(dev.get());
    dlg->setWindowTitle(tr("Imported %1 SVG Plan").arg(stName));

    dlg->resize(300, 200);
    dlg->exec();
}

void SelectStationPage::importSelectedStation()
{
    if(!view->selectionModel()->hasSelection())
        return;

    QModelIndex idx = view->currentIndex();
    db_id stationId = stationsModel->getIdAtRow(idx.row());
    if(!stationId)
        return;

    const QString stName = stationsModel->getNameAtRow(idx.row());

    //Allow changing name
    OwningQPointer<QInputDialog> dlg = new QInputDialog(this);
    dlg->setWindowTitle(tr("Add Station"));
    dlg->setLabelText(tr("Please choose a name for the new station."));
    dlg->setTextValue(stName); //Initial value to original name

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

        QString shortName;
        if(!mWizard->checkNames(stationId, name, shortName))
        {
            QMessageBox::warning(this, tr("Name Already Exists"),
                                 tr("Station name <b>%1 (%2)</b> already exists in current session.<br>"
                                    "Please choose a different name.")
                                     .arg(name, shortName));
            continue; //Second chance
        }

        if(!mWizard->addStation(stationId, name, shortName))
        {
            QMessageBox::warning(this, tr("Import Error"),
                                 tr("Could not import station <b>%1</b>.").arg(name));
            continue; //Second chance
        }

        //Done!
        QMessageBox::information(this, tr("Importation Done"),
                                 tr("Station succesfully imported in current session:<br>"
                                    "Name: <b>%1</b><br>"
                                    "Short: <b>%2</b><br>"
                                    "You can now select another station to import.")
                                     .arg(name, shortName));

        break;
    }
    while (true);

    //Reset selection
    view->clearSelection();
}

void SelectStationPage::startFilterTimer()
{
    stopFilterTimer();
    filterTimerId = startTimer(500);
}

void SelectStationPage::stopFilterTimer()
{
    if(filterTimerId)
    {
        killTimer(filterTimerId);
        filterTimerId = 0;
    }
}

void SelectStationPage::timerEvent(QTimerEvent *e)
{
    if(e->timerId() == filterTimerId)
    {
        updateFilter();
        return;
    }

    QWizardPage::timerEvent(e);
}
