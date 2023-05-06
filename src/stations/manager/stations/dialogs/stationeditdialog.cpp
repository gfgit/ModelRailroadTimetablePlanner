/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stationeditdialog.h"
#include "ui_stationeditdialog.h"

#include "stations/station_name_utils.h"

#include "stations/manager/stations/model/stationgatesmodel.h"
#include "stations/manager/stations/model/stationtracksmodel.h"
#include "stations/manager/stations/model/stationtrackconnectionsmodel.h"

#include "stations/manager/stations/model/stationsvghelper.h"

#include "stations/manager/segments/model/railwaysegmentsmodel.h"
#include "stations/manager/segments/model/railwaysegmenthelper.h"

#include <QHeaderView>
#include "utils/delegates/sql/modelpageswitcher.h"

#include "utils/delegates/combobox/combodelegate.h"
#include "utils/delegates/color/colordelegate.h"
#include "utils/delegates/kmspinbox/spinboxeditorfactory.h"

#include "utils/delegates/sql/sqlfkfielddelegate.h"
#include "stations/match_models/stationtracksmatchmodel.h"
#include "stations/match_models/stationgatesmatchmodel.h"

#include <QMessageBox>

#include <QInputDialog>
#include "newtrackconndlg.h"
#include "stations/manager/segments/dialogs/editrailwaysegmentdlg.h"

#include <QFileDialog>
#include "utils/files/file_format_names.h"
#include "utils/files/recentdirstore.h"

#include "utils/owningqpointer.h"

const QLatin1String station_svg_key = QLatin1String("station_svg_dir");

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
    mDb(db),
    mEnableInternalEdititing(false)
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

    connect(ui->addSVGBut, &QPushButton::clicked, this, &StationEditDialog::addSVGImage);
    connect(ui->remSVGBut, &QPushButton::clicked, this, &StationEditDialog::removeSVGImage);
    connect(ui->saveSVGBut, &QPushButton::clicked, this, &StationEditDialog::saveSVGToFile);
    connect(ui->importConnBut, &QPushButton::clicked, this, &StationEditDialog::importConnFromSVG);
    connect(ui->saveXMLBut, &QPushButton::clicked, this, &StationEditDialog::saveXmlPlan);

    ui->importConnBut->setToolTip(tr("Import connections between gates and station tracks from SVG image.\n"
                                     "NOTE: image must contain track data"));

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

    connect(ui->removeTrackConnBut, &QToolButton::clicked, this, &StationEditDialog::removeSelectedTrackConn);
    connect(ui->addTrackConnBut, &QToolButton::clicked, this, [this]()
            {
                addTrackConnInternal(NewTrackConnDlg::SingleConnection);
            });
    connect(ui->trackToAllGatesBut, &QToolButton::clicked, this, [this]()
            {
                addTrackConnInternal(NewTrackConnDlg::TrackToAllGates);
            });
    connect(ui->gateToAllTracksBut, &QToolButton::clicked, this, [this]()
            {
                addTrackConnInternal(NewTrackConnDlg::GateToAllTracks);
            });

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
    bool hasImage = false;
    if(!gatesModel->getStationInfo(stationName, shortName, type, phoneNumber, hasImage))
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

    updateSVGButtons(hasImage);

    return true;
}

db_id StationEditDialog::getStation() const
{
    return gatesModel->getStation();
}

void StationEditDialog::setStationInternalEditingEnabled(bool enable)
{
    mEnableInternalEdititing = enable;

    //Gates, Tracks, Track connections
    gatesModel->setEditable(enable);
    tracksModel->setEditable(enable);
    trackConnModel->setEditable(enable);

    ui->addGateButton->setEnabled(enable);
    ui->removeGateButton->setEnabled(enable);

    ui->addTrackButton->setEnabled(enable);
    ui->removeTrackButton->setEnabled(enable);
    ui->moveTrackUpBut->setEnabled(enable);
    ui->moveTrackDownBut->setEnabled(enable);

    ui->addTrackConnBut->setEnabled(enable);
    ui->removeTrackConnBut->setEnabled(enable);
    ui->trackToAllGatesBut->setEnabled(enable);
    ui->gateToAllTracksBut->setEnabled(enable);

    //Station Details (but not phone)
    ui->stationNameEdit->setEnabled(enable);
    ui->shortNameEdit->setEnabled(enable);
    ui->stationTypeCombo->setEnabled(enable);

    //SVG Image
    updateSVGButtons(false);
}

void StationEditDialog::setStationExternalEditingEnabled(bool enable)
{
    //Gate connections
    ui->addGateConnBut->setEnabled(enable);
    ui->editGateConnBut->setEnabled(enable);
    ui->removeGateConnBut->setEnabled(enable);

    //Phone number
    ui->phoneEdit->setEnabled(enable);
}

void StationEditDialog::setGateConnectionsVisible(bool enable)
{
    int idx = ui->tabWidget->indexOf(ui->gateConnectionsTab);
    ui->tabWidget->setTabVisible(idx, enable);

    //Refresh model
    gateConnModel->clearCache();
    gateConnModel->refreshData();
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
    OwningQPointer<QInputDialog> dlg = new QInputDialog(this);
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
    OwningQPointer<QInputDialog> dlg = new QInputDialog(this);
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

void StationEditDialog::addTrackConnInternal(int mode)
{
    NewTrackConnDlg::Mode dlgMode = NewTrackConnDlg::Mode(mode);

    QScopedPointer<ISqlFKMatchModel> tracks(trackFactory->createModel());
    QScopedPointer<ISqlFKMatchModel> gates(gatesFactory->createModel());

    OwningQPointer<NewTrackConnDlg> dlg = new NewTrackConnDlg(tracks.get(),
                                                              static_cast<StationGatesMatchModel *>(gates.get()),
                                                              this);
    dlg->setMode(dlgMode);

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

        bool success = false;
        switch (dlgMode)
        {
        case NewTrackConnDlg::SingleConnection:
            success = trackConnModel->addTrackConnection(trackId, trackSide, gateId, gateTrack);
            break;
        case NewTrackConnDlg::TrackToAllGates:
            success = trackConnModel->addTrackToAllGatesOnSide(trackId, trackSide, gateTrack);
            break;
        case NewTrackConnDlg::GateToAllTracks:
            success = trackConnModel->addGateToAllTracks(gateId, gateTrack);
            break;
        }

        if(success)
        {
            break; //Done!
        }
    }
    while (true);
}

void StationEditDialog::updateSVGButtons(bool hasImage)
{
    ui->addSVGBut->setEnabled(!hasImage && mEnableInternalEdititing);
    ui->remSVGBut->setEnabled(hasImage && mEnableInternalEdititing);
    ui->saveSVGBut->setEnabled(hasImage && mEnableInternalEdititing);
    ui->importConnBut->setEnabled(hasImage && mEnableInternalEdititing);
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
    OwningQPointer<EditRailwaySegmentDlg> dlg(new EditRailwaySegmentDlg(mDb, nullptr, this));
    dlg->setSegment(0, getStation(), EditRailwaySegmentDlg::DoNotLock);
    int ret = dlg->exec();
    if(ret != QDialog::Accepted || !dlg)
        return;

    gateConnModel->refreshData(); //Recalc row count
}

void StationEditDialog::editGateConnection()
{
    if(!ui->gateConnView->selectionModel()->hasSelection())
        return;

    db_id segId = gateConnModel->getIdAtRow(ui->gateConnView->currentIndex().row());
    if(!segId)
        return;

    OwningQPointer<EditRailwaySegmentDlg> dlg(new EditRailwaySegmentDlg(mDb, nullptr, this));
    dlg->setSegment(segId, getStation(), EditRailwaySegmentDlg::LockToCurrentValue);
    int ret = dlg->exec();
    if(ret != QDialog::Accepted || !dlg)
        return;

    gateConnModel->refreshData(true); //Refresh fields
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
        QMessageBox::warning(this, tr("Error"), errMsg);
        return;
    }

    gateConnModel->refreshData(); //Recalc row count
}

void StationEditDialog::addSVGImage()
{
    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, tr("Open SVG Image"));
    dlg->setFileMode(QFileDialog::ExistingFile);
    dlg->setAcceptMode(QFileDialog::AcceptOpen);
    dlg->setDirectory(RecentDirStore::getDir(station_svg_key, RecentDirStore::Images));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::svgFile);
    filters << FileFormats::tr(FileFormats::allFiles);
    dlg->setNameFilters(filters);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();

    if(fileName.isEmpty())
        return;

    RecentDirStore::setPath(station_svg_key, fileName);

    QFile f(fileName);
    if(!f.open(QFile::ReadOnly))
    {
        QMessageBox::warning(this, tr("Cannot Read File"), f.errorString());
        return;
    }

    QString errMsg;
    if(!StationSVGHelper::addImage(mDb, getStation(), &f, &errMsg))
    {
        QMessageBox::warning(this, tr("Error Adding SVG"), errMsg);
        return;
    }

    updateSVGButtons(true);
}

void StationEditDialog::removeSVGImage()
{
    int ret = QMessageBox::question(this, tr("Delete Image?"),
                                    tr("Are you sure to delete SVG plan of this station?"));
    if(ret != QMessageBox::Yes)
        return;

    QString errMsg;
    if(!StationSVGHelper::removeImage(mDb, getStation(), &errMsg))
    {
        QMessageBox::warning(this, tr("Error Deleting SVG"), errMsg);
        return;
    }

    updateSVGButtons(false);
}

void StationEditDialog::saveSVGToFile()
{
    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, tr("Save SVG Copy"));
    dlg->setFileMode(QFileDialog::AnyFile);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDirectory(RecentDirStore::getDir(station_svg_key, RecentDirStore::Images));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::svgFile);
    filters << FileFormats::tr(FileFormats::allFiles);
    dlg->setNameFilters(filters);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();
    if(fileName.isEmpty())
        return;

    RecentDirStore::setPath(station_svg_key, fileName);

    QFile f(fileName);
    if(!f.open(QFile::WriteOnly))
    {
        QMessageBox::warning(this, tr("Cannot Save File"), f.errorString());
        return;
    }

    QString errMsg;
    if(!StationSVGHelper::saveImage(mDb, getStation(), &f, &errMsg))
    {
        QMessageBox::warning(this, tr("Error Saving SVG"), errMsg);
    }
}

void StationEditDialog::importConnFromSVG()
{
    std::unique_ptr<QIODevice> dev;
    dev.reset(StationSVGHelper::loadImage(mDb, getStation()));

    if(!dev || !dev->open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("Import Error"),
                             tr("Could not open SVG image. Make sure you added one to this station."));
        return;
    }

    bool ret = StationSVGHelper::importTrackConnFromSVGDev(mDb, getStation(), dev.get());
    if(ret)
    {
        QMessageBox::information(this, tr("Done Importation"),
                                 tr("Track to gate connections have been successfully imported."));
    }
    else
    {
        QMessageBox::warning(this, tr("Import Error"),
                             tr("Generic error"));

    }

    trackConnModel->refreshData(); //Recalc row count
}

void StationEditDialog::saveXmlPlan()
{
    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, tr("Save XML Plan"));
    dlg->setFileMode(QFileDialog::AnyFile);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDirectory(RecentDirStore::getDir(station_svg_key, RecentDirStore::Images));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::xmlFile);
    filters << FileFormats::tr(FileFormats::allFiles);
    dlg->setNameFilters(filters);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();
    if(fileName.isEmpty())
        return;

    RecentDirStore::setPath(station_svg_key, fileName);

    QFile f(fileName);
    if(!f.open(QFile::WriteOnly))
    {
        QMessageBox::warning(this, tr("Cannot Save File"), f.errorString());
        return;
    }

    if(!StationSVGHelper::writeStationXmlFromDB(mDb, getStation(), &f))
    {
        QMessageBox::warning(this, tr("Error Saving XML"), tr("Unknow error"));
    }
}
