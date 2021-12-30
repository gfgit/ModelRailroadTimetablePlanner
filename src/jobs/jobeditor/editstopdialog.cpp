#include "editstopdialog.h"
#include "ui_editstopdialog.h"

#include "model/stopmodel.h"
#include "stopeditinghelper.h"

#include "app/session.h"

#include "utils/jobcategorystrings.h"

#include "rscoupledialog.h"
#include "model/rscouplinginterface.h"

#include "model/rsproxymodel.h"
#include "model/stopcouplingmodel.h"
#include "model/trainassetmodel.h"
#include "model/jobpassingsmodel.h"
#include "jobs/jobsmanager/model/jobshelper.h"

#include "utils/delegates/sql/modelpageswitcher.h"
#include "utils/delegates/sql/customcompletionlineedit.h"

#include <QtMath>

#include "utils/owningqpointer.h"
#include <QMenu>
#include <QMessageBox>

#include "app/scopedebug.h"


EditStopDialog::EditStopDialog(StopModel *m, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditStopDialog),
    stopModel(m),
    readOnly(false)
{
    ui->setupUi(this);

    //Stop
    helper = new StopEditingHelper(Session->m_Db, stopModel,
                                   ui->outGateTrackSpin, ui->arrivalTimeEdit, ui->departureTimeEdit,
                                   this);

    CustomCompletionLineEdit *mStationEdit = helper->getStationEdit();
    CustomCompletionLineEdit *mStTrackEdit = helper->getStTrackEdit();
    CustomCompletionLineEdit *mOutGateEdit = helper->getOutGateEdit();

    ui->curStopLay->setWidget(0, QFormLayout::FieldRole, mStationEdit);
    ui->curStopLay->setWidget(2, QFormLayout::FieldRole, mStTrackEdit);
    ui->curStopLay->setWidget(3, QFormLayout::FieldRole, mOutGateEdit);

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

    connect(ui->editCoupledBut, &QPushButton::clicked, this, &EditStopDialog::editCoupled);
    connect(ui->editUncoupledBut, &QPushButton::clicked, this, &EditStopDialog::editUncoupled);

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

    connect(ui->calcPassingsBut, &QPushButton::clicked, this, &EditStopDialog::calcPassings);

    //BIG TODO: temporarily disable option to Cancel dialog
    //This is because at the moment it doesn't seem Coupling are canceled
    //So you get a mixed state: Arrival/Departure/Descriptio ecc changes are canceled but Coupling changes are still applied
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setReadOnly(true);
}

EditStopDialog::~EditStopDialog()
{
    delete helper;
    helper = nullptr;

    delete ui;
}

void EditStopDialog::clearUi()
{
    helper->stopOutTrackTimer();

    stopIdx = QModelIndex();

    m_jobId = 0;
    m_jobCat = JobCategory::FREIGHT;

    trainAssetModelBefore->setStop(0, QTime(), TrainAssetModel::BeforeStop);
    trainAssetModelAfter->setStop(0, QTime(), TrainAssetModel::AfterStop);

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

void EditStopDialog::setStop(const QModelIndex& idx)
{
    DEBUG_ENTRY;

    if(!idx.isValid())
    {
        clearUi();
        return;
    }

    m_jobId = stopModel->getJobId();
    m_jobCat = stopModel->getCategory();

    stopIdx = idx;

    const StopItem& curStop = stopModel->getItemAt(idx.row());
    StopItem prevStop;
    if(idx.row() == 0)
        prevStop = StopItem(); //First stop has no previous stop
    else
        prevStop = stopModel->getItemAt(idx.row() - 1);

    helper->setStop(curStop, prevStop);

    //Setup Train Asset
    trainAssetModelBefore->setStop(m_jobId, curStop.arrival, TrainAssetModel::BeforeStop);
    trainAssetModelAfter->setStop(m_jobId, curStop.arrival, TrainAssetModel::AfterStop);

    //Hide train asset before stop on First stop
    showBeforeAsset(curStop.type != StopType::First);

    //Hide train asset after stop on Last stop
    showAfterAsset(curStop.type != StopType::Last);

    //Coupling operations
    coupledModel->setStop(curStop.stopId, RsOp::Coupled);
    uncoupledModel->setStop(curStop.stopId, RsOp::Uncoupled);

    //Update UI
    updateInfo();

    //Calc passings
    calcPassings();

    //Update Title
    const QString jobName = JobCategoryName::jobName(m_jobId, m_jobCat);
    setWindowTitle(jobName);
}

void EditStopDialog::updateInfo()
{
    const StopItem& curStop = helper->getCurItem();
    const StopItem& prevStop = helper->getPrevItem();

    const QString inGateStr = helper->getGateString(curStop.fromGate.gateId,
                                                    prevStop.nextSegment.reversed);
    ui->inGateEdit->setText(inGateStr);

    if(curStop.type == StopType::First)
    {
        //Hide box of previous stop
        ui->prevStopBox->setVisible(false);
        ui->curStopBox->setTitle(tr("First Stop"));
    }
    else
    {
        //Show box of previous stop
        ui->prevStopBox->setVisible(true);
        ui->curStopBox->setTitle(curStop.type == StopType::Last ? tr("Last Stop") : tr("Current Stop"));

        QString prevStName;
        if(prevStop.stationId)
        {
            query q(Session->m_Db, "SELECT name FROM stations WHERE id=?");
            q.bind(1, prevStop.stationId);
            q.step();
            prevStName = q.getRows().get<QString>(0);
        }
        ui->prevStEdit->setText(prevStName);

        const QString outGateStr = helper->getGateString(prevStop.toGate.gateId,
                                                         prevStop.nextSegment.reversed);
        ui->prevOutGateEdit->setText(outGateStr);
    }

    const QString descr = stopModel->getDescription(curStop);
    ui->descriptionEdit->setPlainText(descr);

    if(curStop.type == StopType::Transit)
    {
        //On transit you cannot couple/uncouple rollingstock
        ui->editCoupledBut->setEnabled(false);
        ui->editUncoupledBut->setEnabled(false);
    }

    couplingMgr->loadCouplings(stopModel, curStop.stopId, m_jobId, curStop.arrival);
}

void EditStopDialog::saveDataToModel()
{
    DEBUG_ENTRY;

    const StopItem& curStop = helper->getCurItem();
    const StopItem& prevStop = helper->getPrevItem();

    if(ui->descriptionEdit->document()->isModified())
    {
        stopModel->setDescription(stopIdx,
                                  ui->descriptionEdit->toPlainText());
    }

    stopModel->setStopInfo(stopIdx, curStop, prevStop.nextSegment);
}

void EditStopDialog::editCoupled()
{
    const StopItem& curStop = helper->getCurItem();

    coupledModel->clearCache();
    trainAssetModelAfter->clearCache();

    OwningQPointer<RSCoupleDialog> dlg = new RSCoupleDialog(couplingMgr, RsOp::Coupled, this);
    dlg->setWindowTitle(tr("Couple"));
    dlg->loadProxyModels(Session->m_Db, m_jobId, curStop.stopId, curStop.stationId, curStop.arrival);

    dlg->exec();

    coupledModel->refreshData(true);
    trainAssetModelAfter->refreshData(true);
}


void EditStopDialog::editUncoupled()
{
    const StopItem& curStop = helper->getCurItem();

    uncoupledModel->clearCache();
    trainAssetModelAfter->clearCache();

    OwningQPointer<RSCoupleDialog> dlg = new RSCoupleDialog(couplingMgr, RsOp::Uncoupled, this);
    dlg->setWindowTitle(tr("Uncouple"));
    dlg->loadProxyModels(Session->m_Db, m_jobId, curStop.stopId, curStop.stationId, curStop.arrival);

    dlg->exec();

    uncoupledModel->refreshData(true);
    trainAssetModelAfter->refreshData(true);
}

bool EditStopDialog::hasEngineAfterStop()
{
    DEBUG_ENTRY;
    return couplingMgr->hasEngineAfterStop();
}

void EditStopDialog::calcPassings()
{
    DEBUG_ENTRY;

    const StopItem& curStop = helper->getCurItem();

    JobStopDirectionHelper dirHelper(Session->m_Db);
    utils::Side myDirection = dirHelper.getStopOutSide(curStop.stopId);

    query q(Session->m_Db, "SELECT s.id, s.job_id, jobs.category, s.arrival, s.departure,"
                           "t1.name,t2.name"
                           " FROM stops s"
                           " JOIN jobs ON jobs.id=s.job_id"
                           " LEFT JOIN station_gate_connections g1 ON g1.id=s.in_gate_conn"
                           " LEFT JOIN station_gate_connections g2 ON g2.id=s.out_gate_conn"
                           " LEFT JOIN station_tracks t1 ON t1.id=g1.track_id"
                           " LEFT JOIN station_tracks t2 ON t2.id=g2.track_id"
                           " WHERE s.station_id=? AND s.departure >=? AND s.arrival<=? AND s.job_id <> ?");

    q.bind(1, curStop.stationId);
    q.bind(2, curStop.arrival);
    q.bind(3, curStop.arrival);
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

        e.platform = r.get<QString>(6);
        if(e.platform.isEmpty())
            e.platform = r.get<QString>(7); //Use out gate to get track name

        utils::Side otherDir = dirHelper.getStopOutSide(otherStopId);

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
    OwningQPointer<QMenu> menu = new QMenu(this);
    QAction *act = menu->addAction(tr("Refresh"));

    //HACK: could be ui->coupledView or ui->uncoupledView or ui->assetBeforeView or ui->assetAfterView
    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(sender());
    if(!view)
        return; //Error: not called by the view?

    if(menu->exec(view->viewport()->mapToGlobal(pos)) != act)
        return; //User didn't select 'Refresh' action

    //Refresh data
    coupledModel->refreshData(true);
    uncoupledModel->refreshData(true);
    trainAssetModelBefore->refreshData(true);
    trainAssetModelAfter->refreshData(true);
}

int EditStopDialog::getTrainSpeedKmH(bool afterStop)
{
    const StopItem& curStop = helper->getCurItem();

    query q(Session->m_Db, "SELECT MIN(rs_models.max_speed), rs_id FROM("
                           "SELECT coupling.rs_id AS rs_id, MAX(stops.arrival)"
                           " FROM stops"
                           " JOIN coupling ON coupling.stop_id=stops.id"
                           " WHERE stops.job_id=? AND stops.arrival<?"
                           " GROUP BY coupling.rs_id"
                           " HAVING coupling.operation=1)"
                           " JOIN rs_list ON rs_list.id=rs_id"
                           " JOIN rs_models ON rs_models.id=rs_list.model_id");
    q.bind(1, m_jobId); //TODO: maybe move to model

    //HACK: 1 minute is the min interval between stops,
    //by adding 1 minute we include the current stop but leave out the next one
    if(afterStop)
        q.bind(2, curStop.arrival.addSecs(60));
    else
        q.bind(2, curStop.arrival);

    q.step();
    return q.getRows().get<int>(0);
}

void EditStopDialog::setReadOnly(bool value)
{
    readOnly = value;

    CustomCompletionLineEdit *mStationEdit = helper->getStationEdit();
    CustomCompletionLineEdit *mStTrackEdit = helper->getStTrackEdit();
    CustomCompletionLineEdit *mOutGateEdit = helper->getOutGateEdit();

    mStationEdit->setReadOnly(readOnly);
    mStTrackEdit->setReadOnly(readOnly);
    mOutGateEdit->setReadOnly(readOnly);

    ui->arrivalTimeEdit->setReadOnly(readOnly);
    ui->departureTimeEdit->setReadOnly(readOnly);

    ui->descriptionEdit->setReadOnly(readOnly);

    ui->editCoupledBut->setEnabled(!readOnly);
    ui->editUncoupledBut->setEnabled(!readOnly);
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
