#include "stationeditdialog.h"
#include "ui_stationeditdialog.h"

#include "stations/station_name_utils.h"

#include "stations/manager/stations/model/stationgatesmodel.h"
#include "stations/manager/stations/model/stationtracksmodel.h"
#include "stations/manager/stations/model/stationtrackconnectionsmodel.h"

#include "stations/manager/segments/model/railwaysegmentsmodel.h"
#include "stations/manager/segments/model/railwaysegmenthelper.h"

#include <QHeaderView>
#include "utils/sqldelegate/modelpageswitcher.h"

#include "utils/combodelegate.h"
#include "utils/colordelegate.h"
#include "utils/spinbox/spinboxeditorfactory.h"

#include "utils/sqldelegate/sqlfkfielddelegate.h"
#include "stations/match_models/stationtracksmatchmodel.h"
#include "stations/match_models/stationgatesmatchmodel.h"

#include <QMessageBox>

#include <QInputDialog>
#include "newtrackconndlg.h"
#include "stations/manager/segments/dialogs/editrailwaysegmentdlg.h"

#include <QPointer>

void setupView(QTableView *view, IPagedItemModel *model)
{
    //Custom colun sorting
    //NOTE: leave disconnect() in the old SIGLAL()/SLOT() version in order to work
    QHeaderView *header = view->horizontalHeader();
    QObject::disconnect(header, SIGNAL(sectionPressed(int)), view, SLOT(selectColumn(int)));
    QObject::disconnect(header, SIGNAL(sectionEntered(int)), view, SLOT(_q_selectColumn(int)));
    QObject::connect(header, &QHeaderView::sectionClicked, model, [model, header](int section)
                     {
                         model->setSortingColumn(section);
                         header->setSortIndicator(model->getSortingColumn(), Qt::AscendingOrder);
                     });
    header->setSortIndicatorShown(true);
    header->setSortIndicator(model->getSortingColumn(), Qt::AscendingOrder);
    view->setModel(model);
}

StationEditDialog::StationEditDialog(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StationEditDialog),
    mDb(db)
{
    ui->setupUi(this);

    //Enum names
    QStringList stationTypeEnum;
    stationTypeEnum.reserve(int(utils::StationType::NTypes));
    for(int i = 0; i < int(utils::StationType::NTypes); i++)
        stationTypeEnum.append(utils::StationUtils::name(utils::StationType(i)));

    QStringList sideTypeEnum;
    sideTypeEnum.reserve(int(utils::Side::NSides));
    for(int i = 0; i < int(utils::Side::NSides); i++)
        sideTypeEnum.append(utils::StationUtils::name(utils::Side(i)));

    //Delegate Factories
    trackLengthSpinFactory = new SpinBoxEditorFactory;
    trackLengthSpinFactory->setRange(1, 9999);
    trackLengthSpinFactory->setSuffix(" cm");
    trackLengthSpinFactory->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    trackFactory = new StationTracksMatchFactory(mDb, this);
    gatesFactory = new StationGatesMatchFactory(mDb, this);

    //Station Tab
    ui->stationTypeCombo->addItems(stationTypeEnum);

    //Gates Tab
    gatesModel = new StationGatesModel(mDb, this);
    connect(gatesModel, &IPagedItemModel::modelError, this, &StationEditDialog::modelError);
    connect(gatesModel, &StationGatesModel::gateNameChanged, this, &StationEditDialog::onGatesChanged);
    connect(gatesModel, &StationGatesModel::gateRemoved, this, &StationEditDialog::onGatesChanged);

    ModelPageSwitcher *ps = new ModelPageSwitcher(false, this);
    ui->gatesLayout->addWidget(ps);
    ps->setModel(gatesModel);
    setupView(ui->gatesView, gatesModel);

    ui->gatesView->setItemDelegateForColumn(StationGatesModel::SideCol,
                                            new ComboDelegate(sideTypeEnum, Qt::EditRole, this));
    ui->gatesView->setItemDelegateForColumn(StationGatesModel::DefaultInPlatfCol,
                                            new SqlFKFieldDelegate(trackFactory, gatesModel, this));

    connect(ui->addGateButton, &QToolButton::clicked, this, &StationEditDialog::addGate);
    connect(ui->removeGateButton, &QToolButton::clicked, this, &StationEditDialog::removeSelectedGate);

    //Tracks Tab
    tracksModel = new StationTracksModel(mDb, this);
    connect(tracksModel, &IPagedItemModel::modelError, this, &StationEditDialog::modelError);
    connect(tracksModel, &StationTracksModel::trackNameChanged, this, &StationEditDialog::onTracksChanged);
    connect(tracksModel, &StationTracksModel::trackRemoved, this, &StationEditDialog::onTracksChanged);

    ps = new ModelPageSwitcher(false, this);
    ui->tracksLayout->addWidget(ps);
    ps->setModel(tracksModel);
    setupView(ui->trackView, tracksModel);
    ui->trackView->setItemDelegateForColumn(StationTracksModel::ColorCol, new ColorDelegate(this));

    auto trackLengthDelegate = new QStyledItemDelegate(this);
    trackLengthDelegate->setItemEditorFactory(trackLengthSpinFactory);
    ui->trackView->setItemDelegateForColumn(StationTracksModel::TrackLengthCol,    trackLengthDelegate);
    ui->trackView->setItemDelegateForColumn(StationTracksModel::PassengerLegthCol, trackLengthDelegate);
    ui->trackView->setItemDelegateForColumn(StationTracksModel::FreightLengthCol,  trackLengthDelegate);

    connect(ui->addTrackButton, &QToolButton::clicked, this, &StationEditDialog::addTrack);
    connect(ui->removeTrackButton, &QToolButton::clicked, this, &StationEditDialog::removeSelectedTrack);

    ui->moveTrackUpBut->setToolTip(tr("Hold shift to move selected track to the top."));
    ui->moveTrackDownBut->setToolTip(tr("Hold shift to move selected track to the bottom."));
    connect(ui->moveTrackUpBut, &QToolButton::clicked, this, &StationEditDialog::moveTrackUp);
    connect(ui->moveTrackDownBut, &QToolButton::clicked, this, &StationEditDialog::moveTrackDown);

    //Track Connections Tab
    trackConnModel = new StationTrackConnectionsModel(mDb, this);
    connect(trackConnModel, &IPagedItemModel::modelError, this, &StationEditDialog::modelError);
    connect(trackConnModel, &StationTrackConnectionsModel::trackConnRemoved,
            this, &StationEditDialog::onTrackConnRemoved);

    ps = new ModelPageSwitcher(false, this);
    ui->trackConnLayout->addWidget(ps);
    ps->setModel(trackConnModel);
    setupView(ui->trackConnView, trackConnModel);

    ui->trackConnView->setItemDelegateForColumn(StationTrackConnectionsModel::TrackSideCol,
                                                new ComboDelegate(sideTypeEnum, Qt::EditRole, this));
    ui->trackConnView->setItemDelegateForColumn(StationTrackConnectionsModel::TrackCol,
                                                new SqlFKFieldDelegate(trackFactory, trackConnModel, this));
    ui->trackConnView->setItemDelegateForColumn(StationTrackConnectionsModel::GateCol,
                                                new SqlFKFieldDelegate(gatesFactory, trackConnModel, this));

    connect(ui->addTrackConnBut, &QToolButton::clicked, this, &StationEditDialog::addTrackConn);
    connect(ui->removeTrackConnBut, &QToolButton::clicked, this, &StationEditDialog::removeSelectedTrackConn);

    //Gate Connections Tab
    gateConnModel = new RailwaySegmentsModel(mDb, this);
    connect(gateConnModel, &IPagedItemModel::modelError, this, &StationEditDialog::modelError);

    ps = new ModelPageSwitcher(false, this);
    ui->gateConnLayout->addWidget(ps);
    ps->setModel(gateConnModel);
    setupView(ui->gateConnView, gateConnModel);
    //From station column shows always our station, because of filetering, so hide it
    ui->gateConnView->hideColumn(RailwaySegmentsModel::FromStationCol);

    connect(ui->addGateConnBut, &QToolButton::clicked, this, &StationEditDialog::addGateConnection);
    connect(ui->editGateConnBut, &QToolButton::clicked, this, &StationEditDialog::editGateConnection);
    connect(ui->removeGateConnBut, &QToolButton::clicked, this, &StationEditDialog::removeSelectedGateConnection);
}

StationEditDialog::~StationEditDialog()
{
    delete ui;
    delete trackLengthSpinFactory;
}

bool StationEditDialog::setStation(db_id stationId)
{
    //Update models
    if(!gatesModel->setStation(stationId))
        return false;
    tracksModel->setStation(stationId);
    trackConnModel->setStation(stationId);
    gateConnModel->setFilterFromStationId(stationId);

    //Update factories
    trackFactory->setStationId(stationId);
    gatesFactory->setStationId(stationId);

    //Update station details
    QString stationName;
    QString shortName;
    utils::StationType type = utils::StationType::Normal;
    qint64 phoneNumber = -1;
    if(!gatesModel->getStationInfo(stationName, shortName, type, phoneNumber))
        return false;

    ui->stationNameEdit->setText(stationName);
    ui->shortNameEdit->setText(shortName);
    ui->stationTypeCombo->setCurrentIndex(int(type));

    if(phoneNumber == -1)
        ui->phoneEdit->setText(QString());
    else
        ui->phoneEdit->setText(QString::number(phoneNumber));

    //Update title
    setWindowTitle(stationName.isEmpty() ? tr("New Station") : stationName);

    return true;
}

db_id StationEditDialog::getStation() const
{
    return gatesModel->getStation();
}

void StationEditDialog::setStationInternalEditingEnabled(bool enable)
{
    //Gates, Tracks, Track connections
    gatesModel->setEditable(enable);
    tracksModel->setEditable(enable);
    trackConnModel->setEditable(enable);

    //Station Details (but not phone)
    ui->stationNameEdit->setEnabled(enable);
    ui->shortNameEdit->setEnabled(enable);
    ui->stationTypeCombo->setEnabled(enable);

    //TODO: also SVG
}

void StationEditDialog::setStationExternalEditingEnabled(bool enable)
{
    //TODO: Gate connections

    //Phone number
    ui->phoneEdit->setEnabled(enable);
}

void StationEditDialog::done(int res)
{
    if(res == QDialog::Accepted)
    {
        const QString stationName = ui->stationNameEdit->text().simplified();
        const QString shortName = ui->shortNameEdit->text().simplified();
        const utils::StationType type = utils::StationType(ui->stationTypeCombo->currentIndex());

        qint64 phoneNumber = -1;
        const QString phoneNumberStr = ui->phoneEdit->text().simplified();
        if(!phoneNumberStr.isEmpty())
        {
            bool ok = false;
            phoneNumber = phoneNumberStr.toLongLong(&ok);
            if(!ok)
                phoneNumber = -1;
        }

        if(stationName.isEmpty())
        {
            modelError(tr("Station name cannot be empty."));
            return;
        }
        if(stationName == shortName)
        {
            modelError(tr("Station short name cannot be equal to full name.\n"
                          "Leave empty if you want to use the full name in all places."));
            return;
        }
        //FIXME: StationsModel does other checks on names and outputs better error messages
        if(!gatesModel->setStationInfo(stationName, shortName, type, phoneNumber))
        {
            modelError(tr("Check station <b>name</b>, <b>short name</b> and <b>phone number</b> to be <b>unique</b> for this station."));
            return;
        }

        if(!gatesModel->hasAtLeastOneGate())
        {
            //TODO: provide a way to delete the station to exit dialog
            modelError(tr("A station should at least have 1 gate"));
            return;
        }

        if(!tracksModel->hasAtLeastOneTrack())
        {
            modelError(tr("A station should at least have 1 track"));
            return;
        }
    }
    else
    {
        //Warning rejecting dialog
        //TODO: this is because modification to gates/tacks/connections
        //Are applied immediatley so closing dialog always "saves" changes.
        //Hitting cancel one would expect not to see the changes applied.
        //FIXME: find a way to hold changes and apply only if accepting dialog
        modelError(tr("Cannot cancel changes. Changes will be applied."));
    }

    QDialog::done(res);
}

void StationEditDialog::modelError(const QString &msg)
{
    QMessageBox::warning(this, tr("Station Error"), msg);
}

void StationEditDialog::onGatesChanged()
{
    //A gate was removed or changed name

    //Update platform connections
    //(refresh because some may be deleted)
    trackConnModel->refreshData(true);

    //Update gate connections
    gateConnModel->refreshData(true);
}

void StationEditDialog::addGate()
{
    QPointer<QInputDialog> dlg = new QInputDialog(this);
    dlg->setWindowTitle(tr("Add Gate"));
    dlg->setLabelText(tr("Please choose a letter for the new station gate."));
    dlg->setTextValue(QString());

    do{
        int ret = dlg->exec();
        if(ret != QDialog::Accepted || !dlg)
        {
            break; //User canceled
        }

        const QString name = dlg->textValue().simplified();
        if(name.isEmpty())
        {
            QMessageBox::warning(this, tr("Error"), tr("Gate name cannot be empty."));
            continue; //Second chance
        }


        if(gatesModel->addGate(name.at(0)))
        {
            break; //Done!
        }
    }
    while (true);

    delete dlg;
}

void StationEditDialog::removeSelectedGate()
{
    if(!ui->gatesView->selectionModel()->hasSelection())
        return;

    db_id gateId = gatesModel->getIdAtRow(ui->gatesView->currentIndex().row());
    if(!gateId)
        return;

    gatesModel->removeGate(gateId);
}

void StationEditDialog::onTracksChanged()
{
    //A track was removed or changed name

    //Update gates (has a Default Platform column)
    gatesModel->refreshData(true);

    //Update platform connections
    //(refresh because some may be deleted)
    trackConnModel->refreshData(true);
}

void StationEditDialog::addTrack()
{
    QPointer<QInputDialog> dlg = new QInputDialog(this);
    dlg->setWindowTitle(tr("Add Track"));
    dlg->setLabelText(tr("Please choose a name for the new station track."));
    dlg->setTextValue(QString());

    do{
        int ret = dlg->exec();
        if(ret != QDialog::Accepted || !dlg)
        {
            break; //User canceled
        }

        const QString name = dlg->textValue().simplified();
        if(name.isEmpty())
        {
            QMessageBox::warning(this, tr("Error"), tr("Track name cannot be empty."));
            continue; //Second chance
        }


        if(tracksModel->addTrack(-1, name))
        {
            break; //Done!
        }
    }
    while (true);

    delete dlg;
}

void StationEditDialog::removeSelectedTrack()
{
    if(!ui->trackView->selectionModel()->hasSelection())
        return;

    db_id trackId = tracksModel->getIdAtRow(ui->trackView->currentIndex().row());
    if(!trackId)
        return;

    tracksModel->removeTrack(trackId);
}

void StationEditDialog::moveTrackUp()
{
    if(!ui->trackView->selectionModel()->hasSelection())
        return;

    QModelIndex idx = ui->trackView->currentIndex();

    db_id trackId = tracksModel->getIdAtRow(idx.row());
    if(!trackId)
        return;

    bool top = QGuiApplication::keyboardModifiers() & Qt::ShiftModifier;
    if(!tracksModel->moveTrackUpDown(trackId, true, top))
        return;

    idx = idx.siblingAtRow(top ? 0 : idx.row() - 1);
    ui->trackView->setCurrentIndex(idx);
}

void StationEditDialog::moveTrackDown()
{
    if(!ui->trackView->selectionModel()->hasSelection())
        return;

    QModelIndex idx = ui->trackView->currentIndex();

    db_id trackId = tracksModel->getIdAtRow(idx.row());
    if(!trackId)
        return;

    bool bottom = QGuiApplication::keyboardModifiers() & Qt::ShiftModifier;
    if(!tracksModel->moveTrackUpDown(trackId, false, bottom))
        return;

    idx = idx.siblingAtRow(bottom ? tracksModel->rowCount() - 1 : idx.row() + 1);
    ui->trackView->setCurrentIndex(idx);
}

void StationEditDialog::onTrackConnRemoved()
{
    //A track connection was removed

    //Update gates (has a Default Platform column)
    gatesModel->refreshData(true);
}

void StationEditDialog::addTrackConn()
{
    QScopedPointer<ISqlFKMatchModel> tracks(trackFactory->createModel());
    QScopedPointer<ISqlFKMatchModel> gates(gatesFactory->createModel());

    QPointer<NewTrackConnDlg> dlg = new NewTrackConnDlg(tracks.get(), gates.get(), this);

    do{
        int ret = dlg->exec();
        if(ret != QDialog::Accepted || !dlg)
        {
            break; //User canceled
        }

        db_id trackId = 0;
        utils::Side trackSide = utils::Side::East;
        db_id gateId = 0;
        int gateTrack = 0;
        dlg->getData(trackId, trackSide, gateId, gateTrack);

        if(trackConnModel->addTrackConnection(trackId, trackSide, gateId, gateTrack))
        {
            break; //Done!
        }
    }
    while (true);

    delete dlg;
}

void StationEditDialog::removeSelectedTrackConn()
{
    if(!ui->trackConnView->selectionModel()->hasSelection())
        return;

    db_id connId = trackConnModel->getIdAtRow(ui->trackConnView->currentIndex().row());
    if(!connId)
        return;

    trackConnModel->removeTrackConnection(connId);
}

void StationEditDialog::addGateConnection()
{
    QPointer<EditRailwaySegmentDlg> dlg(new EditRailwaySegmentDlg(mDb, this));
    dlg->setSegment(0, getStation(), EditRailwaySegmentDlg::DoNotLock);
    int ret = dlg->exec();
    if(ret != QDialog::Accepted || !dlg)
        return;

    gateConnModel->refreshData(); //Recalc row count
    delete dlg;
}

void StationEditDialog::editGateConnection()
{
    if(!ui->gateConnView->selectionModel()->hasSelection())
        return;

    db_id segId = gateConnModel->getIdAtRow(ui->gateConnView->currentIndex().row());
    if(!segId)
        return;

    QPointer<EditRailwaySegmentDlg> dlg(new EditRailwaySegmentDlg(mDb, this));
    dlg->setSegment(segId, getStation(), EditRailwaySegmentDlg::LockToCurrentValue);
    int ret = dlg->exec();
    if(ret != QDialog::Accepted || !dlg)
        return;

    gateConnModel->refreshData(true); //Refresh fields
    delete dlg;
}

void StationEditDialog::removeSelectedGateConnection()
{
    if(!ui->gateConnView->selectionModel()->hasSelection())
        return;

    db_id segId = gateConnModel->getIdAtRow(ui->gateConnView->currentIndex().row());
    if(!segId)
        return;

    QString errMsg;
    RailwaySegmentHelper helper(mDb);
    if(!helper.removeSegment(segId, &errMsg))
    {
        QMessageBox::warning(this, tr("Error"),
                             tr("Cannot remove segment:\n%1").arg(errMsg));
        return;
    }

    gateConnModel->refreshData(); //Recalc row count
}
