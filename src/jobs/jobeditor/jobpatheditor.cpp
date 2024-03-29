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

#include "jobpatheditor.h"
#include "ui_jobpatheditor.h"

#include "app/session.h"
#include "app/scopedebug.h"

#include "viewmanager/viewmanager.h"

#include "model/stopmodel.h"
#include "stopdelegate.h"
#include "jobs/jobsmanager/model/jobshelper.h"

#include "model/nextprevrsjobsmodel.h"

#include "jobs/jobeditor/editstopdialog.h"

#include "utils/jobcategorystrings.h"

#include "shiftbusy/shiftbusydialog.h"
#include "shiftbusy/shiftbusymodel.h"

#include "odt_export/jobsheetexport.h"
#include "utils/files/openfileinfolder.h"

#include "utils/files/file_format_names.h"

#include "utils/delegates/sql/customcompletionlineedit.h"
#include "shifts/shiftcombomodel.h"

#include "utils/owningqpointer.h"
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include "utils/files/recentdirstore.h"

#include <QCloseEvent>

JobPathEditor::JobPathEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::JobPathEditor),
    stopModel(nullptr),
    jobNumberTimerId(0),
    isClear(true),
    canSetJob(true),
    m_readOnly(false)
{
    ui->setupUi(this);

    QStringList catNames;
    catNames.reserve(int(JobCategory::NCategories));
    for (int cat = 0; cat < int(JobCategory::NCategories); cat++)
    {
        catNames.append(JobCategoryName::fullName(JobCategory(cat)));
    }

    ui->categoryCombo->addItems(catNames);
    ui->categoryCombo->setEditable(false);
    ui->categoryCombo->setCurrentIndex(-1);

    ShiftComboModel *shiftComboModel = new ShiftComboModel(Session->m_Db, this);
    shiftCombo                       = new CustomCompletionLineEdit(shiftComboModel);

    // Get Catecory combo position and insert shift below
    int categoryRow = 0;
    QFormLayout::ItemRole unusedRole;
    ui->formLayout->getWidgetPosition(ui->categoryCombo, &categoryRow, &unusedRole);
    ui->formLayout->insertRow(categoryRow + 1, tr("Shift:"), shiftCombo);

    // Stops
    stopModel = new StopModel(Session->m_Db, this);
    connect(shiftCombo, &CustomCompletionLineEdit::dataIdChanged, stopModel,
            &StopModel::setNewShiftId);
    ui->stopsView->setModel(stopModel);

    delegate = new StopDelegate(Session->m_Db, this);
    ui->stopsView->setItemDelegateForColumn(0, delegate);

    ui->stopsView->setResizeMode(QListView::Adjust);
    ui->stopsView->setMovement(QListView::Static);
    ui->stopsView->setSelectionMode(QListView::ContiguousSelection);

    // Next/Prev Jobs
    prevJobsModel = new NextPrevRSJobsModel(Session->m_Db, this);
    prevJobsModel->setMode(NextPrevRSJobsModel::PrevJobs);
    ui->prevJobsView->setModel(prevJobsModel);

    nextJobsModel = new NextPrevRSJobsModel(Session->m_Db, this);
    nextJobsModel->setMode(NextPrevRSJobsModel::NextJobs);
    ui->nextJobsView->setModel(nextJobsModel);

    connect(ui->categoryCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            stopModel, &StopModel::setCategory);
    connect(stopModel, &StopModel::categoryChanged, this, &JobPathEditor::onCategoryChanged);

    connect(ui->jobIdSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
            &JobPathEditor::startJobNumberTimer);
    connect(stopModel, &StopModel::jobIdChanged, this, &JobPathEditor::onJobIdChanged);

    connect(stopModel, &StopModel::edited, this, &JobPathEditor::setEdited);

    connect(stopModel, &StopModel::errorSetShiftWithoutStops, this, &JobPathEditor::onShiftError);
    connect(stopModel, &StopModel::jobShiftChanged, this, &JobPathEditor::onJobShiftChanged);

    connect(ui->sheetBut, &QPushButton::clicked, this, &JobPathEditor::onSaveSheet);

    ui->stopsView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->stopsView, &QListView::customContextMenuRequested, this,
            &JobPathEditor::showStopsContextMenu);
    connect(ui->stopsView, &QListView::clicked, this, &JobPathEditor::onStopIndexClicked);

    ui->prevJobsView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->prevJobsView, &QListView::customContextMenuRequested, this,
            &JobPathEditor::showJobContextMenu);

    ui->nextJobsView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->nextJobsView, &QListView::customContextMenuRequested, this,
            &JobPathEditor::showJobContextMenu);

    connect(Session, &MeetingSession::jobRemoved, this, &JobPathEditor::onJobRemoved);
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
    if (!canSetJob)
        return false; // We are busy - (Avoid nested loop calls from inside 'saveChanges()')

    if (!isClear && stopModel->getJobId() == jobId)
        return true; // Fake return, we already set this job

    if (isEdited())
    {
        if (!maybeSave())
            return false; // User still wants to edit the current job
    }

    return setJob_internal(jobId);
}

bool JobPathEditor::setJob_internal(db_id jobId)
{
    DEBUG_IMPORTANT_ENTRY;

    if (!canSetJob)
        return false; // We are busy - (Avoid nested loop calls from inside 'saveChanges()')

    if (!isClear && stopModel->getJobId() == jobId)
        return true; // Fake return, we already set this job

    isClear = false;

    stopJobNumberTimer();

    // Load from database
    if (!stopModel->loadJobStops(jobId))
    {
        // Error: job could not be loaded, maybe invalid jobId
        clearJob();
        setEnabled(false);

        QMessageBox::warning(this, tr("Error Loading Job"),
                             tr("<b>Job %1</b> could not be loaded.<br>"
                                "Maybe it's number was changed or maybe it doesn't exist at all.")
                               .arg(jobId));

        return false;
    }

    // If read-only hide 'AddHere' row (last one)
    ui->stopsView->setRowHidden(stopModel->rowCount() - 1, m_readOnly);

    prevJobsModel->setJobId(jobId);
    nextJobsModel->setJobId(jobId);

    return true;
}

void JobPathEditor::startJobNumberTimer()
{
    // Give user a small time to scroll values in ID QSpinBox
    // This will skip eventual non available IDs (already existent)
    // On timeout check ID and reset to old value if not available
    stopJobNumberTimer();
    jobNumberTimerId = startTimer(700);
}

void JobPathEditor::stopJobNumberTimer()
{
    if (jobNumberTimerId)
    {
        killTimer(jobNumberTimerId);
        jobNumberTimerId = 0;
    }
}

void JobPathEditor::checkJobNumberValid()
{
    // Kill timer
    stopJobNumberTimer();

    db_id jobId = ui->jobIdSpin->value();
    if (!stopModel->setNewJobId(jobId))
    {
        QMessageBox::warning(this, tr("Invalid"),
                             tr("Job number <b>%1</b> is already exists.<br>"
                                "Please choose a different number.")
                               .arg(jobId));
    }
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

    if (out)
        *out = 0;

    if (!clearJob())
        return false; // Busy JobPathEditor

    db_id jobId = 0;
    if (!JobsHelper::createNewJob(Session->m_Db, jobId) || jobId == 0)
    {
        return false; // An error occurred in database, abort
    }

    if (!setJob_internal(jobId))
    {
        // If we fail opening JobPathEditor remove the job
        JobsHelper::removeJob(Session->m_Db, jobId);
        return false;
    }

    if (out)
        *out = jobId;

    return true;
}

void JobPathEditor::showStopsContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->stopsView->indexAt(pos);
    if (!index.isValid() || index.row() >= stopModel->rowCount() || stopModel->isAddHere(index))
        return;

    OwningQPointer<QMenu> menu = new QMenu(this);
    QAction *toggleTransitAct  = menu->addAction(tr("Toggle transit"));
    QAction *setToTransitAct   = menu->addAction(tr("Set transit"));
    QAction *unsetTransit      = menu->addAction(tr("Unset transit"));
    menu->insertSeparator(unsetTransit);
    QAction *editStopAct    = menu->addAction(tr("Edit stop"));
    QAction *showStationSVG = menu->addAction(tr("Station SVG Plan"));
    menu->insertSeparator(editStopAct);
    QAction *removeStopAct = menu->addAction(tr("Remove"));

    toggleTransitAct->setEnabled(!m_readOnly);
    setToTransitAct->setEnabled(!m_readOnly);
    unsetTransit->setEnabled(!m_readOnly);
    removeStopAct->setEnabled(!m_readOnly);

    const StopItem stop = stopModel->getItemAt(index.row());
    showStationSVG->setEnabled(stop.stationId != 0); // Enable only if station is set

    QAction *act            = menu->exec(ui->stopsView->viewport()->mapToGlobal(pos));

    QItemSelectionModel *sm = ui->stopsView->selectionModel();

    QItemSelectionRange range;
    QItemSelection s = ui->stopsView->selectionModel()->selection();
    if (s.count() > 0)
    {
        // Take the first range only
        range = s.at(0); // Save range for later
    }

    // Select only 1 index
    sm->select(index, QItemSelectionModel::ClearAndSelect);

    if (act == editStopAct)
    {
        OwningQPointer<EditStopDialog> dlg = new EditStopDialog(stopModel, this);
        dlg->setReadOnly(m_readOnly);
        dlg->setStop(index);
        dlg->exec();
        return;
    }

    if (act == showStationSVG)
    {
        Session->getViewManager()->requestStSVGPlan(stop.stationId, true, stop.arrival);
    }

    if (m_readOnly)
        return;

    if (range.isValid())
    {
        StopType type = StopType::ToggleType;
        bool useRange = true;
        if (act == toggleTransitAct)
            type = StopType::ToggleType;
        else if (act == setToTransitAct)
            type = StopType::Transit;
        else if (act == unsetTransit)
            type = StopType::Normal;
        else
            useRange = false;

        if (useRange)
        {
            stopModel->setStopTypeRange(range.top(), range.bottom(), type);
            // Select only the range we changed (unselect possible other indexes)
            sm->select(QItemSelection(range.topLeft(), range.bottomRight()),
                       QItemSelectionModel::ClearAndSelect);
            return;
        }
    }

    if (act == removeStopAct)
    {
        stopModel->removeStop(index);
    }
}

void JobPathEditor::showJobContextMenu(const QPoint &pos)
{
    QTableView *jobView           = qobject_cast<QTableView *>(sender());
    NextPrevRSJobsModel *jobModel = nextJobsModel;

    if (jobView == ui->prevJobsView)
        jobModel = prevJobsModel;

    QModelIndex index              = jobView->indexAt(pos);

    NextPrevRSJobsModel::Item item = jobModel->getItemAtRow(index.row());

    OwningQPointer<QMenu> menu     = new QMenu(this);
    QAction *goToStop              = menu->addAction(tr("Go to Stop"));
    QAction *goToJob               = menu->addAction(tr("Show Job"));
    QAction *showRSPlan            = menu->addAction(tr("Show RS Plan"));
    menu->addSeparator();
    QAction *refreshViews = menu->addAction(tr("Refresh"));

    // Enable only if RS is not going to depot
    goToStop->setEnabled(index.isValid());
    goToJob->setEnabled(index.isValid() && item.otherJobId != 0);
    showRSPlan->setEnabled(index.isValid());

    QAction *act = menu->exec(jobView->viewport()->mapToGlobal(pos));

    if (act == goToStop)
    {
        selectStop(item.stopId);
    }
    else if (act == goToJob)
    {
        if (isEdited()) // Prevent selecting other job before saving
        {
            if (!maybeSave())
                return;
        }
        Session->getViewManager()->requestJobSelection(item.otherJobId, true, true);
    }
    else if (act == showRSPlan)
    {
        Session->getViewManager()->requestRSInfo(item.rsId);
    }
    else if (act == refreshViews)
    {
        prevJobsModel->refreshData();
        nextJobsModel->refreshData();
    }
}

bool JobPathEditor::clearJob()
{
    DEBUG_ENTRY;

    if (!canSetJob)
        return false;

    if (isEdited())
    {
        if (!maybeSave())
            return false;
    }

    isClear = true;

    // Reset color
    ui->jobIdSpin->setPalette(QPalette());

    stopModel->clearJob();

    stopJobNumberTimer();

    prevJobsModel->clearData();
    nextJobsModel->clearData();

    return true;
}

void JobPathEditor::done(int res)
{
    if (res == Accepted)
    {
        // Accepted: save changes
        if (!saveChanges())
            return; // Give user a second chance to edit job
    }
    else
    {
        // Rejected: discard changes
        discardChanges();
    }

    // NOTE: if we call QDialog::done() the dialog is closed and QDockWidget remains open but empty
    setResult(res);
    if (res == QDialog::Accepted)
        emit accepted();
    else if (res == QDialog::Rejected)
        emit rejected();
    emit finished(res);
}

bool JobPathEditor::saveChanges()
{
    DEBUG_IMPORTANT_ENTRY;

    if (!canSetJob)
        return false;
    canSetJob = false;

    closeStopEditor();

    checkJobNumberValid();

    stopModel->removeLastIfEmpty();
    stopModel->uncoupleStillCoupledAtLastStop();

    if (stopModel->rowCount() < 3) // At least 2 stops + AddHere
    {
        int res = QMessageBox::warning(this, tr("Error"),
                                       tr("You must register at least 2 stops.\n"
                                          "Do you want to delete this job?"),
                                       QMessageBox::Yes | QMessageBox::No);

        if (res == QMessageBox::Yes)
        {
            qDebug() << "User wants to delete job:" << stopModel->getJobId();
            stopModel->commitChanges();
            JobsHelper::removeJob(Session->m_Db, stopModel->getJobId());
            canSetJob = true;
            clearJob();
            setEnabled(false);
            return true;
        }
        canSetJob = true;

        return false; // Give user a second chance
    }

    // Re-check shift because user may have added stops so this job could last longer!
    if (stopModel->getNewShiftId())
    {
        auto times = stopModel->getFirstLastTimes();

        ShiftBusyModel model(Session->m_Db);
        model.loadData(stopModel->getNewShiftId(), stopModel->getJobId(), times.first,
                       times.second);
        if (model.hasConcurrentJobs())
        {
            OwningQPointer<ShiftBusyDlg> dlg = new ShiftBusyDlg(this);
            dlg->setModel(&model);
            dlg->exec();

            stopModel->setNewShiftId(stopModel->getJobShiftId());

            canSetJob = true;
            return false;
        }
    }

    stopModel->commitChanges();

    db_id newJobId = stopModel->getJobId();
    prevJobsModel->setJobId(newJobId);
    nextJobsModel->setJobId(newJobId);

    // When updating the path selection gets cleared so we restore it
    Session->getViewManager()->requestJobSelection(newJobId, true, true);

    canSetJob = true;
    return true;
}

void JobPathEditor::discardChanges()
{
    DEBUG_ENTRY;

    if (!canSetJob)
        return;

    canSetJob = false;

    closeStopEditor(); // Close before rolling savepoint

    stopJobNumberTimer();

    stopModel->revertChanges(); // Re-load old job from db

    // After re-load but before possible 'clearJob()' (Below)
    // Because this hides 'AddHere' so after 'loadJobStops()'
    // Before 'clearJob()' because sets isEdited = false and
    //'maybeSave()' doesn't be called in an infinite loop

    canSetJob = true;

    if (stopModel->rowCount() < 3) // At least 2 stops + AddHere
    {
        // User discarded an invalid job so we delete it
        // This usually happens when you create a new job but then you change your mind and press
        // 'Discard'
        qDebug() << "User wants to delete job:" << stopModel->getJobId();
        stopModel->commitChanges();
        JobsHelper::removeJob(Session->m_Db, stopModel->getJobId());
        clearJob();
        setEnabled(false);
    }
}

db_id JobPathEditor::currentJobId() const
{
    return stopModel->getJobId();
}

bool JobPathEditor::maybeSave()
{
    DEBUG_ENTRY;
    QMessageBox::StandardButton ret = QMessageBox::question(
      this, tr("Save?"), tr("Do you want to save changes to job %1").arg(stopModel->getJobId()),
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
    if (!isClear)
    {
        QColor col = Session->colorForCat(stopModel->getCategory());
        setSpinColor(col);
    }
}

void JobPathEditor::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == jobNumberTimerId)
    {
        checkJobNumberValid();
        return;
    }

    QDialog::timerEvent(e);
}

void JobPathEditor::onJobRemoved(db_id jobId)
{
    // If the job shown is about to be removed clear JobPathEditor
    if (stopModel->getJobId() == jobId)
    {
        if (clearJob())
            setEnabled(false);
    }
}

void JobPathEditor::onJobIdChanged(db_id jobId)
{
    ui->jobIdSpin->setValue(int(jobId));
}

void JobPathEditor::setSpinColor(const QColor &col)
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
    shiftCombo->setData(shiftId);

    if (shiftId)
    {
        auto times = stopModel->getFirstLastTimes();

        ShiftBusyModel model(Session->m_Db);
        model.loadData(shiftId, stopModel->getJobId(), times.first, times.second);
        if (model.hasConcurrentJobs())
        {
            OwningQPointer<ShiftBusyDlg> dlg = new ShiftBusyDlg(this);
            dlg->setModel(&model);
            dlg->exec();

            stopModel->setNewShiftId(0);

            return;
        }
    }
}

void JobPathEditor::onShiftError()
{
    QMessageBox::warning(this, tr("Empty Job"),
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
    if (m_readOnly == readOnly)
        return;

    m_readOnly = readOnly;

    ui->jobIdSpin->setReadOnly(m_readOnly);
    ui->categoryCombo->setDisabled(m_readOnly);
    shiftCombo->setDisabled(m_readOnly);

    ui->buttonBox->setVisible(!m_readOnly);

    // If read-only hide 'AddHere' row (last one)
    int size = stopModel->rowCount();
    if (size > 0)
        ui->stopsView->setRowHidden(size - 1, m_readOnly);

    if (m_readOnly)
    {
        ui->stopsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
    else
    {
        ui->stopsView->setEditTriggers(QAbstractItemView::DoubleClicked);
    }
}

void JobPathEditor::onSaveSheet()
{
    const QLatin1String job_sheet_key = QLatin1String("job_sheet_dir");

    OwningQPointer<QFileDialog> dlg   = new QFileDialog(this, tr("Save Job Sheet"));
    dlg->setFileMode(QFileDialog::AnyFile);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDirectory(RecentDirStore::getDir(job_sheet_key, RecentDirStore::Documents));
    dlg->selectFile(tr("job%1_sheet.odt").arg(stopModel->getJobId()));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::odtFormat);
    dlg->setNameFilters(filters);

    if (dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();

    if (fileName.isEmpty())
        return;

    RecentDirStore::setPath(job_sheet_key, fileName);

    JobSheetExport sheet(stopModel->getJobId(), stopModel->getCategory());
    sheet.write();
    sheet.save(fileName);

    utils::OpenFileInFolderDlg::askUser(tr("Job Sheet Saved"), fileName, this);
}

void JobPathEditor::onStopIndexClicked(const QModelIndex &index)
{
    DEBUG_ENTRY;

    if (m_readOnly)
        return;

    if (stopModel->isAddHere(index))
    {
        qDebug() << index << "AddHere";

        stopModel->addStop();

        int row = index.row();

        if (row > 0)
        {
            // idx - 1 is former Last Stop (now it became a normal Stop)
            // idx     is new Last Stop (former AddHere)
            // idx + 1 is the new AddHere

            // Edit former Last Stop
            QModelIndex prev = stopModel->index(row - 1, 0);
            ui->stopsView->setCurrentIndex(prev);
            ui->stopsView->scrollTo(prev);
            ui->stopsView->edit(prev);

            // Tell editor to popup lines combo
            // QAbstractItemView::edit doesn't let you pass additional arguments
            // So we work around by emitting a signal
            // See 'StopDelegate::createEditor()'
            emit delegate->popupEditorSegmentCombo();
        }
        else
        {
            QModelIndex lastStop = stopModel->index(row, 0);
            ui->stopsView->setCurrentIndex(lastStop);
            ui->stopsView->scrollTo(lastStop);
            ui->stopsView->edit(lastStop);
        }
    }
}

bool JobPathEditor::getCanSetJob() const
{
    return canSetJob;
}

void JobPathEditor::closeStopEditor()
{
    QModelIndex idx = ui->stopsView->currentIndex();
    QWidget *ed     = ui->stopsView->indexWidget(idx);
    if (ed == nullptr)
        return;
    emit delegate->commitData(ed);
    emit delegate->closeEditor(ed);
}

void JobPathEditor::closeEvent(QCloseEvent *e)
{
    // TODO: prevent QDockWidget closing even if we ignore this event
    if (isEdited())
    {
        if (maybeSave())
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
    if (row >= 0)
    {
        QModelIndex idx = stopModel->index(row, 0);
        ui->stopsView->setCurrentIndex(idx);
        ui->stopsView->scrollTo(idx, QListView::EnsureVisible);
    }
}
