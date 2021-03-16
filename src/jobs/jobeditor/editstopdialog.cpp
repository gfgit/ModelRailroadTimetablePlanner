#include "editstopdialog.h"
#include "ui_editstopdialog.h"

#include "app/session.h"

#include <QMenu>

#include "app/scopedebug.h"

#include "model/stopmodel.h"

#include "lines/helpers.h"
#include "utils/jobcategorystrings.h"

#include <QMessageBox>

#include <QtMath>

#include "utils/platform_utils.h"

#include "rscoupledialog.h"
#include "model/rscouplinginterface.h"

#include "model/rsproxymodel.h"
#include "model/stopcouplingmodel.h"
#include "model/trainassetmodel.h"
#include "model/jobpassingsmodel.h"

#include "utils/sqldelegate/modelpageswitcher.h"
#include "utils/sqldelegate/customcompletionlineedit.h"

#include "stations/stationsmatchmodel.h"

EditStopDialog::EditStopDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditStopDialog),
    readOnly(false)
{
    ui->setupUi(this);

    stationsMatchModel = new StationsMatchModel(Session->m_Db, this);
    stationLineEdit = new CustomCompletionLineEdit(stationsMatchModel, this);
    ui->stopBoxLayout->addWidget(stationLineEdit, 0, 1);

    //Coupling
    couplingMgr = new RSCouplingInterface(Session->m_Db, this);

    coupledModel = new StopCouplingModel(Session->m_Db, this);
    auto ps = new ModelPageSwitcher(true, this);
    ps->setModel(coupledModel);
    ui->coupledView->setModel(coupledModel);
    ui->coupledLayout->insertWidget(1, ps);

    uncoupledModel = new StopCouplingModel(Session->m_Db, this);
    ps = new ModelPageSwitcher(true, this);
    ps->setModel(uncoupledModel);
    ui->uncoupledView->setModel(uncoupledModel);
    ui->uncoupledLayout->insertWidget(1, ps);

    ui->coupledView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->uncoupledView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->coupledView, &QAbstractItemView::customContextMenuRequested, this, &EditStopDialog::couplingCustomContextMenuRequested);
    connect(ui->uncoupledView, &QAbstractItemView::customContextMenuRequested, this, &EditStopDialog::couplingCustomContextMenuRequested);

    //Setup train asset models
    trainAssetModelBefore = new TrainAssetModel(Session->m_Db, this);
    ps = new ModelPageSwitcher(true, this);
    ps->setModel(trainAssetModelBefore);
    ui->assetBeforeView->setModel(trainAssetModelBefore);
    ui->trainAssetGridLayout->addWidget(ps, 2, 0);

    trainAssetModelAfter = new TrainAssetModel(Session->m_Db, this);
    ps = new ModelPageSwitcher(true, this);
    ps->setModel(trainAssetModelAfter);
    ui->assetAfterView->setModel(trainAssetModelAfter);
    ui->trainAssetGridLayout->addWidget(ps, 2, 1);

    ui->assetBeforeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->assetAfterView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->assetBeforeView, &QAbstractItemView::customContextMenuRequested, this, &EditStopDialog::couplingCustomContextMenuRequested);
    connect(ui->assetAfterView, &QAbstractItemView::customContextMenuRequested, this, &EditStopDialog::couplingCustomContextMenuRequested);

    //Setup Crossings/Passings
    passingsModel = new JobPassingsModel(this);
    ui->passingsView->setModel(passingsModel);

    crossingsModel = new JobPassingsModel(this);
    ui->crossingsView->setModel(crossingsModel);

    connect(stationLineEdit, &CustomCompletionLineEdit::dataIdChanged, this, &EditStopDialog::onStEditingFinished);

    connect(ui->editCoupledBut, &QPushButton::clicked, this, &EditStopDialog::editCoupled);
    connect(ui->editUncoupledBut, &QPushButton::clicked, this, &EditStopDialog::editUncoupled);

    connect(ui->calcPassingsBut, &QPushButton::clicked, this, &EditStopDialog::calcPassings);

    connect(ui->calcTimeBut, &QPushButton::clicked, this, &EditStopDialog::calcTime);
    connect(ui->applyTimeBut, &QPushButton::clicked, this, &EditStopDialog::applyTime);

    connect(ui->platfRadio, &QRadioButton::toggled, this, &EditStopDialog::onPlatfRadioToggled);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    //BIG TODO: temporarily disable option to Cancel dialog
    //This is because at the moment it doesn't seem Coupling are canceled
    //So you get a mixed state: Arrival/Departure/Descriptio ecc changes are canceled but Coupling changes are still applied
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);

    setReadOnly(true);
}

EditStopDialog::~EditStopDialog()
{
    delete ui;
}

void EditStopDialog::clearUi()
{
    m_stopId = 0;
    stopIdx = QModelIndex();

    m_jobId = 0;

    m_prevStId = 0;

    trainAssetModelBefore->setStop(0, QTime(), TrainAssetModel::BeforeStop);
    trainAssetModelAfter->setStop(0, QTime(), TrainAssetModel::AfterStop);

    originalArrival = QTime();
    originalDeparture = QTime();

    curLine = 0;
    curSegment = 0;

    //TODO: clear UI properly
}

void EditStopDialog::showBeforeAsset(bool val)
{
    ui->assetBeforeView->setVisible(val);
    ui->assetBeforeLabel->setVisible(val);
}

void EditStopDialog::showAfterAsset(bool val)
{
    ui->assetAfterView->setVisible(val);
    ui->assetAfterLabel->setVisible(val);
}

void EditStopDialog::setStop(StopModel *stops, const QModelIndex& idx)
{
    DEBUG_ENTRY;

    if(!idx.isValid())
    {
        clearUi();
        return;
    }

    stopIdx = idx;
    stopModel = stops;

    m_jobId = idx.data(JOB_ID_ROLE).toLongLong();
    m_jobCat = JobCategory(idx.data(JOB_CATEGORY_ROLE).toInt());

    m_stopId = idx.data(STOP_ID).toLongLong();
    m_stationId = idx.data(STATION_ID).toLongLong();

    stopType = StopType(idx.data(STOP_TYPE_ROLE).toInt());

    if(idx.row() == 0) //First stop
    {
        m_prevStId = 0;

        showBeforeAsset(false); //Hide train asset before stop
        showAfterAsset(true);
    }
    else //Not First stop
    {
        QModelIndex prevIdx = idx.model()->index(idx.row() - 1, idx.column());
        previousDep = prevIdx.data(DEP_ROLE).toTime();
        m_prevStId = prevIdx.data(STATION_ID).toLongLong();

        //Cannot arrive at same time (or before) the previous departure, minimum travel duration of one minute
        //This reflects StopModel behaviour when editing stops with StopEditor
        QTime minArrival = previousDep.addSecs(60);
        ui->arrivalTimeEdit->setMinimumTime(minArrival);
        ui->calcTimeArr->setMinimumTime(minArrival);

        if(stopType == Normal)
            minArrival = minArrival.addSecs(60); //At least 1 minute stop for normal stops
        ui->departureTimeEdit->setMinimumTime(minArrival);
        ui->calcTimeDep->setMinimumTime(minArrival);

        showBeforeAsset(true);

        if (idx.row() == stopModel->rowCount() - 2) //Last stop (size - 1 - AddHere)
        {
            showAfterAsset(false); //Hide train asset after stop
        }
        else //A stop in the middle
        {
            showAfterAsset(true);
        }
    }


    const QString jobName = JobCategoryName::jobName(m_jobId, m_jobCat);
    setWindowTitle(jobName);

    originalArrival = idx.data(ARR_ROLE).toTime();
    originalDeparture = idx.data(DEP_ROLE).toTime();

    curSegment = idx.data(SEGMENT_ROLE).toLongLong();
    curLine = idx.data(CUR_LINE_ROLE).toLongLong();

    stationsMatchModel->setFilter(curLine, m_prevStId);

    coupledModel->setStop(m_stopId, RsOp::Coupled);
    uncoupledModel->setStop(m_stopId, RsOp::Uncoupled);

    trainAssetModelBefore->setStop(m_jobId, originalArrival, TrainAssetModel::BeforeStop);
    trainAssetModelAfter->setStop(m_jobId, originalArrival, TrainAssetModel::AfterStop);

    updateInfo();

    calcPassings();
}

void EditStopDialog::updateInfo()
{
    ui->arrivalTimeEdit->setTime(originalArrival);
    ui->departureTimeEdit->setTime(originalDeparture);

    stationLineEdit->setData(m_stationId);

    int platf = stopIdx.data(PLATF_ID).toInt();

    int platfCount = 0;
    int depotCount = 0;
    stopModel->getStationPlatfCount(m_stationId, platfCount, depotCount);
    setPlatformCount(platfCount, depotCount);
    setPlatform(platf);

    //Show previous station if any
    if(m_prevStId)
    {
        query q(Session->m_Db, "SELECT name FROM stations WHERE id=?");
        q.bind(1, m_prevStId);
        q.step();
        ui->prevStEdit->setText(q.getRows().get<QString>(0));
    }

    const QString descr = stopIdx.data(STOP_DESCR_ROLE).toString();

    ui->descriptionEdit->setPlainText(descr);

    if(stopType == First)
    {
        ui->arrivalTimeEdit->setEnabled(false);
        ui->calcTimeArr->setEnabled(false);

        ui->departureTimeEdit->setEnabled(true);
        ui->calcTimeDep->setEnabled(true);
    }
    else if (stopType == Last || stopType == Transit || stopType == TransitLineChange)
    {
        ui->departureTimeEdit->setEnabled(false);
        ui->calcTimeDep->setEnabled(false);

        ui->arrivalTimeEdit->setEnabled(true);
        ui->calcTimeArr->setEnabled(true);
    }
    else
    {
        ui->arrivalTimeEdit->setEnabled(true);
        ui->calcTimeArr->setEnabled(true);
        ui->departureTimeEdit->setEnabled(true);
        ui->calcTimeDep->setEnabled(true);
    }

    if(stopType == Transit || stopType == TransitLineChange)
    {
        //On transit you cannot couple/uncouple rollingstock
        ui->editCoupledBut->setEnabled(false);
        ui->editUncoupledBut->setEnabled(false);
    }

    couplingMgr->loadCouplings(stopModel, m_stopId, m_jobId, originalArrival);

    speedBeforeStop = getTrainSpeedKmH(false);

    updateSpeedAfterStop();
    //Save original speed after stop to compare it after new coupling operations
    originalSpeedAfterStopKmH = newSpeedAfterStopKmH; //Initially there is no difference

    updateDistance();
}

void EditStopDialog::setPlatformCount(int maxMainPlatf, int maxDepots)
{
    ui->mainPlatfSpin->setMaximum(maxMainPlatf);
    ui->depotSpin->setMaximum(maxDepots);

    ui->platfRadio->setEnabled(maxMainPlatf);
    ui->mainPlatfSpin->setEnabled(maxMainPlatf);

    ui->depotSpin->setEnabled(maxDepots);
    ui->depotRadio->setEnabled(maxDepots);
}

void EditStopDialog::setPlatform(int platf)
{
    if(platf < 0)
    {
        ui->platfRadio->setChecked(false);
        ui->mainPlatfSpin->setEnabled(false);
        ui->mainPlatfSpin->setValue(0);

        ui->depotRadio->setChecked(true);
        ui->depotSpin->setEnabled(true);
        ui->depotSpin->setValue( - platf); //Negative num --> to positive
    }
    else
    {
        ui->platfRadio->setChecked(true);
        ui->mainPlatfSpin->setEnabled(true);
        ui->mainPlatfSpin->setValue(platf + 1); //DB starts from platf 0 --> change to 1

        ui->depotRadio->setChecked(false);
        ui->depotSpin->setEnabled(false);
        ui->depotSpin->setValue(0);
    }
}

int EditStopDialog::getPlatform()
{
    if(ui->platfRadio->isChecked())
        return ui->mainPlatfSpin->value() - 1; //Positive from 0
    else
        return -ui->depotSpin->value(); //Negative from -1
}

void EditStopDialog::onStEditingFinished(db_id stationId)
{
    DEBUG_ENTRY;

    m_stationId = stationId;

    int platfCount = 0;
    int depotCount = 0;
    stopModel->getStationPlatfCount(m_stationId, platfCount, depotCount);
    setPlatformCount(platfCount, depotCount);

    //Fix platform
    int platf = stopModel->data(stopIdx, PLATF_ID).toInt();
    if(platf < 0 && -platf > depotCount)
    {
        if(depotCount)
            platf = -depotCount; //Max depot platform
        else
            platf = 0; //If there arent depots, use platform 0 (First main platform)
    }
    else if(platf >= platfCount)
    {
        if(platfCount)
            platf = platfCount - 1; //Max main platform
        else
            platf = -1; //If there are no main platforms, use -1 (First depot platform)
    }

    //Update UI for platform
    setPlatform(platf);

    updateDistance();
}

void EditStopDialog::saveDataToModel()
{
    DEBUG_ENTRY;

    stopModel->setData(stopIdx, m_stationId, STATION_ID);

    int platf = getPlatform();
    stopModel->setData(stopIdx, platf, PLATF_ID);

    if(ui->descriptionEdit->document()->isModified())
    {
        stopModel->setData(stopIdx,
                           ui->descriptionEdit->toPlainText(),
                           STOP_DESCR_ROLE);
    }

    stopModel->setData(stopIdx, ui->arrivalTimeEdit->time(), ARR_ROLE);
    stopModel->setData(stopIdx, ui->departureTimeEdit->time(), DEP_ROLE);
}

void EditStopDialog::updateDistance()
{
    DEBUG_ENTRY;

    if(stopType == First)
    {
        ui->infoLabel->setText(tr("This is the first stop"));
        return;
    }

    query q(Session->m_Db, "SELECT name FROM stations WHERE id=?");
    q.bind(1, m_prevStId);
    q.step();

    QString res = tr("Train leaves %1 at %2")
            .arg(q.getRows().get<QString>(0))
            .arg(previousDep.toString("HH:mm"));
    if(stopType == Last)
        res.append(tr("\nThis is the last stop"));
    ui->infoLabel->setText(res);

    query q_getLineNameAndSpeed(Session->m_Db, "SELECT name,max_speed FROM lines WHERE id=?");
    q_getLineNameAndSpeed.bind(1, curLine);
    if(q_getLineNameAndSpeed.step() != SQLITE_ROW)
    {
        //Error
    }
    auto r = q_getLineNameAndSpeed.getRows();
    QString lineName = r.get<QString>(0);
    int lineSpeedKmH = r.get<int>(1);

    ui->currentLineText->setText(lineName);
    ui->lineSpeedBox->setValue(lineSpeedKmH);
}

void EditStopDialog::editCoupled()
{
    coupledModel->clearCache();
    trainAssetModelAfter->clearCache();

    RSCoupleDialog dlg(couplingMgr, RsOp::Coupled, this);
    dlg.setWindowTitle(tr("Couple"));
    dlg.loadProxyModels(Session->m_Db, m_jobId, m_stopId, m_stationId, ui->arrivalTimeEdit->time());

    dlg.exec();

    coupledModel->refreshData();
    trainAssetModelAfter->refreshData();
    updateSpeedAfterStop();
}


void EditStopDialog::editUncoupled()
{
    uncoupledModel->clearCache();
    trainAssetModelAfter->clearCache();

    RSCoupleDialog dlg(couplingMgr, RsOp::Uncoupled, this);
    dlg.setWindowTitle(tr("Uncouple"));
    dlg.loadProxyModels(Session->m_Db, m_jobId, m_stopId, m_stationId, originalArrival);

    dlg.exec();

    uncoupledModel->refreshData();
    trainAssetModelAfter->refreshData();
    updateSpeedAfterStop();
}

bool EditStopDialog::hasEngineAfterStop()
{
    DEBUG_ENTRY;
    return couplingMgr->hasEngineAfterStop();
}

void EditStopDialog::calcPassings()
{
    DEBUG_ENTRY;

    Direction myDirection = Session->getStopDirection(m_stopId, m_stationId);

    query q(Session->m_Db, "SELECT s.id, s.jobid, j.category, s.arrival, s.departure, s.platform"
                           " FROM stops s"
                           " JOIN jobs j ON j.id=s.jobId"
                           " WHERE s.stationId=? AND s.departure >=? AND s.arrival<=? AND s.jobId <> ?");

    q.bind(1, m_stationId);
    q.bind(2, originalArrival);
    q.bind(3, originalDeparture);
    q.bind(4, m_jobId);

    QVector<JobPassingsModel::Entry> passings, crossings;

    for(auto r : q)
    {
        JobPassingsModel::Entry e;

        db_id otherStopId = r.get<db_id>(0);
        e.jobId = r.get<db_id>(1);
        e.category = JobCategory(r.get<int>(2));
        e.arrival = r.get<QTime>(3);
        e.departure = r.get<QTime>(4);
        e.platform = r.get<int>(5);

        Direction otherDir = Session->getStopDirection(otherStopId, m_stationId);

        if(myDirection == otherDir)
            passings.append(e); //Same direction -> Passing
        else
            crossings.append(e); //Opposite direction -> Crossing
    }

    q.reset();

    passingsModel->setJobs(passings);
    crossingsModel->setJobs(crossings);

    ui->passingsView->resizeColumnsToContents();
    ui->crossingsView->resizeColumnsToContents();
}

void EditStopDialog::couplingCustomContextMenuRequested(const QPoint& pos)
{
    QMenu menu(this);
    QAction *act = menu.addAction(tr("Refresh"));

    //HACK: could be ui->coupledView or ui->uncoupledView or ui->assetBeforeView or ui->assetAfterView
    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(sender());
    if(!view)
        return; //Error: not called by the view?

    if(menu.exec(view->viewport()->mapToGlobal(pos)) != act)
        return; //User didn't select 'Refresh' action

    //Refresh data
    coupledModel->refreshData();
    coupledModel->clearCache();

    uncoupledModel->refreshData();
    uncoupledModel->clearCache();

    trainAssetModelBefore->refreshData();
    trainAssetModelBefore->clearCache();

    trainAssetModelAfter->refreshData();
    trainAssetModelAfter->clearCache();
}

QSet<db_id> EditStopDialog::getRsToUpdate() const
{
    return rsToUpdate; //TODO: fill when coupling/uncoupling/canceling operations
}

int EditStopDialog::getTrainSpeedKmH(bool afterStop)
{
    query q(Session->m_Db, "SELECT MIN(rs_models.max_speed), rsId FROM("
                           "SELECT coupling.rsId AS rsId, MAX(stops.arrival)"
                           " FROM stops"
                           " JOIN coupling ON coupling.stopId=stops.id"
                           " WHERE stops.jobId=? AND stops.arrival<?"
                           " GROUP BY coupling.rsId"
                           " HAVING coupling.operation=1)"
                           " JOIN rs_list ON rs_list.id=rsId"
                           " JOIN rs_models ON rs_models.id=rs_list.model_id");
    q.bind(1, m_jobId); //TODO: maybe move to model

    //HACK: 1 minute is the min interval between stops,
    //by adding 1 minute we include the current stop but leave out the next one
    if(afterStop)
        q.bind(2, originalArrival.addSecs(60));
    else
        q.bind(2, originalArrival);

    q.step();
    return q.getRows().get<int>(0);
}

void EditStopDialog::updateSpeedAfterStop()
{
    newSpeedAfterStopKmH = getTrainSpeedKmH(true);
    ui->trainSpeedBox->setValue(newSpeedAfterStopKmH);

    const int lineSpeedKmH = ui->lineSpeedBox->value();
    const int maxSpeedKmH = qMin(newSpeedAfterStopKmH, lineSpeedKmH);

    ui->speedEdit->setValue(maxSpeedKmH);
}

void EditStopDialog::calcTime()
{
    DEBUG_IMPORTANT_ENTRY;

    double maxSpeedKmH = ui->speedEdit->value();

    if(maxSpeedKmH == 0.0)
    {
        qDebug() << "Err!";
        return;
    }

    const double distanceMeters = lines::getStationsDistanceInMeters(Session->m_Db, curLine, m_stationId, m_prevStId);

    QTime time = previousDep;

    const double secs = (distanceMeters / maxSpeedKmH) * 3.6;
    time = time.addSecs(qCeil(secs));
    qDebug() << "Arrival:" << time;

    ui->calcTimeArr->setTime(time);

    QTime curArr = ui->arrivalTimeEdit->time();
    QTime curDep = ui->departureTimeEdit->time();

    time = time.addSecs(curArr.secsTo(curDep));
    ui->calcTimeDep->setTime(time);
}

void EditStopDialog::applyTime()
{
    ui->arrivalTimeEdit->setTime(ui->calcTimeArr->time());
    ui->departureTimeEdit->setTime(ui->calcTimeDep->time());
}

void EditStopDialog::setReadOnly(bool value)
{
    readOnly = value;

    stationLineEdit->setReadOnly(readOnly);
    ui->arrivalTimeEdit->setReadOnly(readOnly);
    ui->departureTimeEdit->setReadOnly(readOnly);

    ui->platfRadio->setEnabled(!readOnly);
    ui->depotRadio->setEnabled(!readOnly);
    ui->mainPlatfSpin->setReadOnly(readOnly);
    ui->depotSpin->setReadOnly(readOnly);

    ui->descriptionEdit->setReadOnly(readOnly);

    ui->editCoupledBut->setEnabled(!readOnly);
    ui->editUncoupledBut->setEnabled(!readOnly);

    ui->applyTimeBut->setEnabled(!readOnly);
}

void EditStopDialog::onPlatfRadioToggled(bool enable)
{
    DEBUG_ENTRY;
    ui->mainPlatfSpin->setEnabled(enable);
    ui->depotSpin->setDisabled(enable);
}

void EditStopDialog::done(int val)
{
    if(val == QDialog::Accepted)
    {
        if(stopIdx.row() < stopModel->rowCount() - 2)
        {
            //We are not last stop

            //Check if train has at least one engine after this stop
            //But not if we are Last stop (size - 1 - AddHere)
            //because the train doesn't have to leave the station
            bool electricOnNonElectrifiedLine = false;
            if(!couplingMgr->hasEngineAfterStop(&electricOnNonElectrifiedLine) || electricOnNonElectrifiedLine)
            {
                int ret = QMessageBox::warning(this,
                                               tr("No Engine Left"),
                                               electricOnNonElectrifiedLine ?
                                                   tr("It seems you have uncoupled all job engines except for electric ones "
                                                      "but the line is not electrified\n"
                                                      "(The train isn't able to move)\n"
                                                      "Do you want to couple a non electric engine?") :
                                                   tr("It seems you have uncoupled all job engines\n"
                                                      "(The train isn't able to move)\n"
                                                      "Do you want to couple an engine?"),
                                               QMessageBox::Yes | QMessageBox::No);

                if(ret == QMessageBox::Yes)
                {
                    return; //Second chance to edit couplings
                }
            }

#ifdef ENABLE_AUTO_TIME_RECALC
            if(originalSpeedAfterStop != newSpeedAfterStop)
            {
                int speedBefore = originalSpeedAfterStop;
                int speedAfter = newSpeedAfterStop;

                LinesModel *linesModel = stopModel->getLinesModel();
                db_id lineId = curLine ? curLine : stopIdx.data(NEXT_LINE_ROLE).toLongLong();
                int lineSpeed = linesModel->getLineSpeed(lineId);
                if(speedBefore == 0)
                {
                    //If speed is null (likely because there weren't RS coupled before)
                    //Fall back to line max speed
                    speedBefore = lineSpeed;
                }
                if(speedAfter == 0)
                {
                    //If speed is null (likely because there isn't RS coupled after this stop)
                    //Fall back to line max speed
                    speedAfter = lineSpeed;
                }
                int ret = QMessageBox::question(this,
                                                tr("Train Speed Changed"),
                                                tr("Train speed after this stop has changed from a value of %1 km/h to <b>%2 km/h</b>\n"
                                                   "Do you want to rebase travel times to this new speed?\n"
                                                   "NOTE: this doesn't affect stop times but you will lose manual adjustments to travel times")
                                                .arg(speedBefore).arg(speedAfter),
                                                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);

                if(ret == QMessageBox::Cancel)
                {
                    return; //Second chance to edit couplings
                }

                if(ret == QMessageBox::Yes)
                {
                    stopModel->rebaseTimesToSpeed(stopIdx.row(), ui->arrivalTimeEdit->time(), ui->departureTimeEdit->time());
                }
            }
#endif
        }

        saveDataToModel();
    }

    QDialog::done(val);
}
