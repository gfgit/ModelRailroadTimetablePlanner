#include "jobpatheditor.h"
#include "ui_jobpatheditor.h"

#include "app/session.h"
#include "app/scopedebug.h"

#include "viewmanager/viewmanager.h"

#include <QFileDialog>
#include <QStandardPaths>

#include <QMessageBox>

#include <QCloseEvent>

#include "model/stopmodel.h"
#include "stopdelegate.h"

#include "jobs/jobeditor/editstopdialog.h"

#include "jobs/jobstorage.h"
#include "utils/jobcategorystrings.h"

#include "shiftbusy/shiftbusydialog.h"
#include "shiftbusy/shiftbusymodel.h"

#include "odt_export/jobsheetexport.h"

#ifdef ENABLE_RS_CHECKER
#include "backgroundmanager/backgroundmanager.h"
#include "rollingstock/rs_checker/rscheckermanager.h"
#endif

#include "utils/file_format_names.h"

#include "utils/sqldelegate/customcompletionlineedit.h"
#include "shifts/shiftcombomodel.h"

JobPathEditor::JobPathEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::JobPathEditor),
    stopModel(nullptr),
    isClear(true),
    canSetJob(true),
    m_readOnly(false)
{
    ui->setupUi(this);

    QStringList catNames;
    catNames.reserve(int(JobCategory::NCategories));
    for(int cat = 0; cat < int(JobCategory::NCategories); cat++)
    {
        catNames.append(JobCategoryName::tr(JobCategoryFullNameTable[cat]));
    }

    ui->categoryCombo->addItems(catNames);
    ui->categoryCombo->setEditable(false);
    ui->categoryCombo->setCurrentIndex(-1);

    ShiftComboModel *shiftComboModel = new ShiftComboModel(Session->m_Db, this);
    shiftCustomCombo = new CustomCompletionLineEdit(shiftComboModel);
    ui->verticalLayout->insertWidget(ui->verticalLayout->indexOf(ui->categoryCombo) + 1, shiftCustomCombo);

    stopModel = new StopModel(Session->m_Db, this);
    connect(shiftCustomCombo, &CustomCompletionLineEdit::dataIdChanged, stopModel, &StopModel::setNewShiftId);
    ui->view->setModel(stopModel);

    delegate = new StopDelegate(Session->m_Db, this);
    ui->view->setItemDelegateForColumn(0, delegate);

    ui->view->setResizeMode(QListView::Adjust);
    ui->view->setMovement(QListView::Static);
    ui->view->setSelectionMode(QListView::ContiguousSelection);

    connect(ui->categoryCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), stopModel, &StopModel::setCategory);
    connect(stopModel, &StopModel::categoryChanged, this, &JobPathEditor::onCategoryChanged);

    connect(ui->jobIdSpin, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &JobPathEditor::onIdSpinValueChanged);
    connect(stopModel, &StopModel::jobIdChanged, this, &JobPathEditor::onJobIdChanged);

    connect(stopModel, &StopModel::edited, this, &JobPathEditor::setEdited);

    connect(stopModel, &StopModel::errorSetShiftWithoutStops, this, &JobPathEditor::onShiftError);
    connect(stopModel, &StopModel::jobShiftChanged, this, &JobPathEditor::onJobShiftChanged);

    connect(ui->sheetBut, &QPushButton::clicked, this, &JobPathEditor::onSaveSheet);

    ui->view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->view, &QListView::customContextMenuRequested, this, &JobPathEditor::showContextMenu);
    connect(ui->view, &QListView::clicked, this, &JobPathEditor::onIndexClicked);

    //Connect to stationsModel to update station views
    //NOTE: here we use queued connections to avoid freezing the UI if there are many stations to update
    //      with queued connections they are update one at a time and the UI stays responsive
    connect(this, &JobPathEditor::stationChange, Session, &MeetingSession::stationPlanChanged, Qt::QueuedConnection);

    connect(Session->mJobStorage, &JobStorage::aboutToRemoveJob, this, &JobPathEditor::onJobRemoved);
    connect(&AppSettings, &MRTPSettings::jobColorsChanged, this, &JobPathEditor::updateSpinColor);

    setReadOnly(false);
    setEdited(false);

    setMinimumWidth(340);
}

JobPathEditor::~JobPathEditor()
{
    clearJob();

    delete ui;
}

bool JobPathEditor::setJob(db_id jobId)
{
    if(!canSetJob)
        return false; //We are busy - (Avoid nested loop calls from inside 'saveChanges()')

    if(!isClear && stopModel->getJobId() == jobId)
        return true; //Fake return, we already set this job

    if(isEdited())
    {
        if(!maybeSave())
            return false; //User still wants to edit the current job
    }

    return setJob_internal(jobId);
}

bool JobPathEditor::setJob_internal(db_id jobId)
{
    DEBUG_IMPORTANT_ENTRY;

    if(!canSetJob)
        return false; //We are busy - (Avoid nested loop calls from inside 'saveChanges()')

    if(!isClear && stopModel->getJobId() == jobId)
        return true; //Fake return, we already set this job

    isClear = false;

    stopModel->loadJobStops(jobId); //Load from database

    //If read-only hide 'AddHere' row (last one)
    ui->view->setRowHidden(stopModel->rowCount() - 1, m_readOnly);

    return true;
}

bool JobPathEditor::createNewJob(db_id *out)
{
    /* Creates a new job and fills UI
     *
     * A newly created job is always invalid because you haven't
     * added stops to it yet, so it isn't shown in any line graph.
     * Therefore if it is'n shown you can't delete it or edit it because
     * you cannot click it.
     *
     * To avoid leaving invalid jobs in the database without user knowing it
     * we remove automatically the job if we cannot open JobPathEditor
     * or if user 'Cancel' the JobPathEditor
     * or if user doesn't add at least 2 stops to the job
     */

    if(out)
        *out = 0;

    if(!clearJob())
        return false; //Busy JobPathEditor

    JobStorage *jobs = Session->mJobStorage;

    db_id jobId = 0;
    jobs->addJob(&jobId); //Request add job

    if(jobId == 0)
    {
        return false; //An error occurred in database, abort
    }

    if(!setJob_internal(jobId))
    {
        //If we fail opening JobPathEditor remove the job
        //Call directly to the model.
        jobs->removeJob(jobId);
        return false;
    }

    if(out)
        *out = jobId;


    return true;
}

void JobPathEditor::toggleTransit(const QModelIndex& index)
{
    DEBUG_ENTRY;

    if(m_readOnly)
        return;

    StopType type = StopDelegate::getStopType(index);
    if(type == First || type == Last)
        return;

    if(type == Transit || type == TransitLineChange)
    {
        type = Normal;
    }
    else
    {
        db_id nextLineId = index.data(NEXT_LINE_ROLE).toLongLong();
        if (nextLineId != 0) {
            type = TransitLineChange;
        }
        else
        {
            type = Transit;
        }
    }

    int err = stopModel->setStopType(index, type);

    if(err == StopModel::ErrorTransitWithCouplings)
    {
        QMessageBox::warning(this,
                             tr("Invalid Operation"),
                             tr("Transit cannot have coupling or uncoupling operations.\n"
                                "Remove theese operation to set Transit on this stop."));
    }
}

void JobPathEditor::showContextMenu(const QPoint& pos)
{
    QModelIndex index = ui->view->indexAt(pos);
    if(!index.isValid() || stopModel->isAddHere(index))
        return;

    QMenu menu(this);
    QAction *toggleTransitAct = menu.addAction(tr("Toggle transit"));
    QAction *setToTransitAct = menu.addAction(tr("Set transit"));
    QAction *unsetTransit = menu.addAction(tr("Unset transit"));
    QAction *removeStopAct = menu.addAction(tr("Remove"));
    QAction *insertBeforeAct = menu.addAction(tr("Insert before"));
    QAction *editStopAct = menu.addAction(tr("Edit stop"));

    toggleTransitAct->setEnabled(!m_readOnly);
    setToTransitAct->setEnabled(!m_readOnly);
    unsetTransit->setEnabled(!m_readOnly);
    removeStopAct->setEnabled(!m_readOnly);
    insertBeforeAct->setEnabled(!m_readOnly);

    QAction *act = menu.exec(ui->view->viewport()->mapToGlobal(pos));

    QItemSelectionModel *sm = ui->view->selectionModel();

    QItemSelectionRange range;
    QItemSelection s = ui->view->selectionModel()->selection();
    if(s.count() > 0)
    {
        //Take the first range only
        range = s.at(0); //Save range for later
    }

    //Select only 1 index
    sm->select(index, QItemSelectionModel::ClearAndSelect);

    if(act == editStopAct)
    {
        EditStopDialog dlg(this);
        dlg.setReadOnly(m_readOnly);
        dlg.setStop(stopModel, index);
        dlg.exec();
        return;
    }

    if(m_readOnly)
        return;

    if(range.isValid())
    {
        StopType type = StopType::ToggleType;
        bool useRange = true;
        if(act == toggleTransitAct)
            type = StopType::ToggleType;
        else if(act == setToTransitAct)
            type = StopType::Transit;
        else if(act == unsetTransit)
            type = StopType::Normal;
        else
            useRange = false;

        if(useRange)
        {
            stopModel->setStopTypeRange(range.top(), range.bottom(), type);
            //Select only the range we changed (unselect possible other indexes)
            sm->select(QItemSelection(range.topLeft(), range.bottomRight()), QItemSelectionModel::ClearAndSelect);
            return;
        }
    }
    else if(act == toggleTransitAct)
    {
        toggleTransit(index);
        return;
    }

    if(act == removeStopAct)
    {
        stopModel->removeStop(index);
    }
    else if(act == insertBeforeAct)
    {
        stopModel->insertStopBefore(index);
    }
}

bool JobPathEditor::clearJob()
{
    DEBUG_ENTRY;

    if(!canSetJob)
        return false;

    if(isEdited())
    {
        if(!maybeSave())
            return false;
    }

    isClear = true;

    //Reset color
    ui->jobIdSpin->setPalette(QPalette());

    stopModel->clearJob();

    return true;
}

void JobPathEditor::prepareQueries()
{
    stopModel->prepareQueries();
}

void JobPathEditor::finalizeQueries()
{
    stopModel->finalizeQueries();
}

void JobPathEditor::done(int res)
{
    if(res == Accepted)
    {
        //Accepted: save changes
        if(!saveChanges())
            return; //Give user a second chance to edit job
    }
    else
    {
        //Rejected: discard changes
        discardChanges();
    }

    //NOTE: if we call QDialog::done() the dialog is closed and QDockWidget remains open but empty
    setResult(res);
    if(res == QDialog::Accepted)
        emit accepted();
    else if(res == QDialog::Rejected)
        emit rejected();
    emit finished(res);
}

bool JobPathEditor::saveChanges()
{
    DEBUG_IMPORTANT_ENTRY;

    if(!canSetJob)
        return false;
    canSetJob = false;

    closeStopEditor();

    stopModel->removeLastIfEmpty();
    stopModel->uncoupleStillCoupledAtLastStop();

    if(stopModel->rowCount() < 3) //At least 2 stops + AddHere
    {
        int res = QMessageBox::warning(this,
                                       tr("Error"),
                                       tr("You must register at least 2 stops.\n"
                                          "Do you want to delete this job?"),
                                       QMessageBox::Yes | QMessageBox::No);

        if(res == QMessageBox::Yes)
        {
            qDebug() << "User wants to delete job:" << stopModel->getJobId();
            stopModel->commitChanges();
            Session->mJobStorage->removeJob(stopModel->getJobId());
            canSetJob = true;
            clearJob();
            setEnabled(false);
            return true;
        }
        canSetJob = true;

        return false; //Give user a second chance
    }

    //Re-check shift because user may have added stops so this job could last longer!
    if(stopModel->getNewShiftId())
    {
        auto times = stopModel->getFirstLastTimes();

        ShiftBusyModel model(Session->m_Db);
        model.loadData(stopModel->getNewShiftId(),
                       stopModel->getJobId(),
                       times.first, times.second);
        if(model.hasConcurrentJobs())
        {
            ShiftBusyDlg dlg(this);
            dlg.setModel(&model);
            dlg.exec();

            stopModel->setNewShiftId(stopModel->getJobShiftId());

            canSetJob = true;
            return false;
        }
    }

    JobStorage *jobs = Session->mJobStorage;
    jobs->updateFirstLast(stopModel->getJobId());

    //Update views
    Session->getViewManager()->updateRSPlans(stopModel->getRsToUpdate());

#ifdef ENABLE_RS_CHECKER
    //Check RS for errors
    if(AppSettings.getCheckRSOnJobEdit())
        Session->getBackgroundManager()->getRsChecker()->checkRs(stopModel->getRsToUpdate());
#endif

    //Update station views
    auto stations = stopModel->getStationsToUpdate();
    for(db_id stId : stations)
    {
        emit stationChange(stId);
    }

    stopModel->commitChanges();

    //TODO: redraw graphs

    //When updating the path selection gets cleared so we restore it
    Session->getViewManager()->requestJobSelection(stopModel->getJobId(), true, true);

    canSetJob = true;
    return true;
}

void JobPathEditor::discardChanges()
{
    DEBUG_ENTRY;

    if(!canSetJob)
        return;

    canSetJob = false;

    closeStopEditor(); //Close before rolling savepoint

    //Save them before reverting changes
    QSet<db_id> rsToUpdate = stopModel->getRsToUpdate();
    QSet<db_id> stToUpdate = stopModel->getStationsToUpdate();

    stopModel->revertChanges(); //Re-load old job from db

    //After re-load but before possible 'clearJob()' (Below)
    //Because this hides 'AddHere' so after 'loadJobStops()'
    //Before 'clearJob()' because sets isEdited = false and
    //'maybeSave()' doesn't be called in an infinite loop

    canSetJob = true;

    if(stopModel->rowCount() < 3) //At least 2 stops + AddHere
    {
        //User discarded an invalid job so we delete it
        //This usually happens when you create a new job but then you change your mind and press 'Discard'
        qDebug() << "User wants to delete job:" << stopModel->getJobId();
        stopModel->commitChanges();
        Session->mJobStorage->removeJob(stopModel->getJobId());
        clearJob();
        setEnabled(false);
    }

    //After possible job deletion update views

    //Update RS views
    Session->getViewManager()->updateRSPlans(rsToUpdate);

#ifdef ENABLE_RS_CHECKER
    //Check RS for errors
    if(AppSettings.getCheckRSOnJobEdit())
        Session->getBackgroundManager()->getRsChecker()->checkRs(rsToUpdate);
#endif

    //Update station views
    for(db_id stId : stToUpdate)
    {
        emit stationChange(stId);
    }
}

db_id JobPathEditor::currentJobId() const
{
    return stopModel->getJobId();
}

bool JobPathEditor::maybeSave()
{
    DEBUG_ENTRY;
    QMessageBox::StandardButton ret = QMessageBox::question(this,
                                                            tr("Save?"),
                                                            tr("Do you want to save changes to job %1")
                                                            .arg(stopModel->getJobId()),
                                                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    switch (ret)
    {
    case QMessageBox::Yes:
        return saveChanges();
    case QMessageBox::No:
        discardChanges();
        return true;
    default:
        break;
    }
    return false;
}

void JobPathEditor::updateSpinColor()
{
    if(!isClear)
    {
        QColor col = Session->colorForCat(stopModel->getCategory());
        setSpinColor(col);
    }
}

void JobPathEditor::onJobRemoved(db_id jobId)
{
    //If the job shown is about to be removed clear JobPathEditor
    if(stopModel->getJobId() == jobId)
    {
        if(clearJob())
            setEnabled(false);
    }
}

void JobPathEditor::onIdSpinValueChanged(int jobId)
{
    if(!stopModel->setNewJobId(jobId))
    {
        QMessageBox::warning(this, tr("Invalid"),
                             tr("Job number <b>%1</b> is already exists.<br>"
                                "Please choose a different number.").arg(jobId));
    }
}

void JobPathEditor::onJobIdChanged(db_id jobId)
{
    ui->jobIdSpin->setValue(int(jobId));
}

void JobPathEditor::setSpinColor(const QColor& col)
{
    QPalette pal = ui->jobIdSpin->palette();
    pal.setColor(QPalette::Text, col);
    ui->jobIdSpin->setPalette(pal);
}

void JobPathEditor::onCategoryChanged(int newCat)
{
    ui->categoryCombo->setCurrentIndex(newCat);
    updateSpinColor();
}

void JobPathEditor::onJobShiftChanged(db_id shiftId)
{
    shiftCustomCombo->setData(shiftId);

    if(shiftId)
    {
        auto times = stopModel->getFirstLastTimes();

        ShiftBusyModel model(Session->m_Db);
        model.loadData(shiftId,
                       stopModel->getJobId(),
                       times.first, times.second);
        if(model.hasConcurrentJobs())
        {
            ShiftBusyDlg dlg(this);
            dlg.setModel(&model);
            dlg.exec();

            stopModel->setNewShiftId(0);

            return;
        }
    }
}

void JobPathEditor::onShiftError()
{
    QMessageBox::warning(this,
                         tr("Empty Job"),
                         tr("Before setting a shift you should add stops to this job"),
                         QMessageBox::Ok);
}

bool JobPathEditor::isEdited() const
{
    return stopModel->isEdited();
}

void JobPathEditor::setEdited(bool val)
{
    ui->buttonBox->setEnabled(val);
    ui->sheetBut->setEnabled(!val);
}

void JobPathEditor::setReadOnly(bool readOnly)
{
    if(m_readOnly == readOnly)
        return;

    m_readOnly = readOnly;

    ui->jobIdSpin->setReadOnly(m_readOnly);
    ui->categoryCombo->setDisabled(m_readOnly);
    shiftCustomCombo->setDisabled(m_readOnly);

    ui->buttonBox->setVisible(!m_readOnly);

    //If read-only hide 'AddHere' row (last one)
    int size = stopModel->rowCount();
    if(size > 0)
        ui->view->setRowHidden(size - 1, m_readOnly);

    if(m_readOnly)
    {
        ui->view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
    else
    {
        ui->view->setEditTriggers(QAbstractItemView::DoubleClicked);
    }
}

void JobPathEditor::onSaveSheet()
{
    QFileDialog dlg(this, tr("Save Job Sheet"));
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    dlg.selectFile(tr("job%1_sheet.odt").arg(stopModel->getJobId()));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::odtFormat);
    dlg.setNameFilters(filters);

    if(dlg.exec() != QDialog::Accepted)
        return;

    QString fileName = dlg.selectedUrls().value(0).toLocalFile();

    if(fileName.isEmpty())
        return;

    JobSheetExport sheet(stopModel->getJobId(), stopModel->getCategory());
    sheet.write();
    sheet.save(fileName);
}

void JobPathEditor::onIndexClicked(const QModelIndex& index)
{
    DEBUG_ENTRY;

    if(m_readOnly)
        return;

    int addHere = index.data(ADDHERE_ROLE).toInt();
    if(addHere == 1)
    {
        qDebug() << index << "AddHere";

        stopModel->addStop();

        int row = index.row();
        if(row > 0 && AppSettings.getChooseLineOnAddStop())
        {
            //idx - 1 is former Last Stop (now it became a normal Stop)
            //idx     is new Last Stop (former AddHere)
            //idx + 1 is the new AddHere

            //Edit former Last Stop
            QModelIndex prev = stopModel->index(row - 1, 0);
            ui->view->setCurrentIndex(prev);
            ui->view->scrollTo(prev);
            ui->view->edit(prev);

            //Tell editor to popup lines combo
            //QAbstractItemView::edit doesn't let you pass additional arguments
            //So we work around by emitting a signal
            //See 'StopDelegate::createEditor()'
            delegate->popupEditorLinesCombo();
        }
        else
        {
            QModelIndex lastStop = stopModel->index(row, 0);
            ui->view->setCurrentIndex(lastStop);
            ui->view->scrollTo(lastStop);
            ui->view->edit(lastStop);
        }
    }
}

bool JobPathEditor::getCanSetJob() const
{
    return canSetJob;
}

void JobPathEditor::closeStopEditor()
{
    QModelIndex idx = ui->view->currentIndex();
    QWidget *ed = ui->view->indexWidget(idx);
    if(ed == nullptr)
        return;
    delegate->commitData(ed);
    delegate->closeEditor(ed);
}

void JobPathEditor::closeEvent(QCloseEvent *e)
{
    //TODO: prevent QDockWidget closing even if we ignore this event
    if(isEdited())
    {
        if(maybeSave())
            e->accept();
        else
            e->ignore();
    }
    else
    {
        e->accept();
    }
}

void JobPathEditor::selectStop(db_id stopId)
{
    int row = stopModel->getStopRow(stopId);
    if(row >= 0)
    {
        QModelIndex idx = stopModel->index(row, 0);
        ui->view->setCurrentIndex(idx);
        ui->view->scrollTo(idx, QListView::EnsureVisible);
    }
}
