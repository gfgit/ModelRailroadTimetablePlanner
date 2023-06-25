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

#include "shiftmanager.h"

#include "shiftsmodel.h"
#include "utils/delegates/sql/modelpageswitcher.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include "app/scopedebug.h"

#include <QVBoxLayout>

#include <QFileDialog>
#include "utils/files/recentdirstore.h"
#include <QMessageBox>
#include <QInputDialog>
#include "utils/owningqpointer.h"

#include <QTableView>
#include "utils/delegates/sql/filterheaderview.h"
#include <QToolBar>
#include <QActionGroup>

#include "odt_export/shiftsheetexport.h"
#include "utils/files/openfileinfolder.h"

#include "utils/files/file_format_names.h"

ShiftManager::ShiftManager(QWidget *parent) :
    QWidget(parent),
    m_readOnly(false),
    edited(false)
{
    QVBoxLayout *l = new QVBoxLayout(this);

    toolBar        = new QToolBar(this);
    l->addWidget(toolBar);

    view = new QTableView(this);
    view->setSelectionMode(QTableView::SingleSelection);
    l->addWidget(view);

    // Custom filtering
    FilterHeaderView *header = new FilterHeaderView(view);
    header->installOnTable(view);

    auto ps = new ModelPageSwitcher(false, this);
    l->addWidget(ps);

    QFont f = view->font();
    f.setPointSize(12);
    view->setFont(f);

    model = new ShiftsModel(Session->m_Db, this);
    model->refreshData();
    ps->setModel(model);
    view->setModel(model);

    act_New    = toolBar->addAction(tr("New"), this, &ShiftManager::onNewShift);
    act_Remove = toolBar->addAction(tr("Remove"), this, &ShiftManager::onRemoveShift);
    toolBar->addSeparator();
    act_displayShift = toolBar->addAction(tr("View Shift"), this, &ShiftManager::onViewShift);
    act_Sheet        = toolBar->addAction(tr("Sheet"), this, &ShiftManager::onSaveSheet);
    toolBar->addSeparator();
    act_Graph   = toolBar->addAction(tr("Graph"), this, &ShiftManager::displayGraph);

    actionGroup = new QActionGroup(this);
    actionGroup->addAction(act_New);
    actionGroup->addAction(act_Remove);

    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &ShiftManager::onShiftSelectionChanged);
    connect(model, &QAbstractItemModel::modelReset, this, &ShiftManager::onShiftSelectionChanged);
    connect(model, &ShiftsModel::modelError, this, &ShiftManager::onModelError);

    setReadOnly(false);

    // Action Tooltips
    act_New->setToolTip(tr("Create new Job Shift"));
    act_Remove->setToolTip(tr("Remove selected Job Shift"));
    act_displayShift->setToolTip(tr("Show selected Job Shift plan"));
    act_Sheet->setToolTip(tr("Save selected Job Shift plan on file"));
    act_Graph->setToolTip(tr("Show Job Shift graph"));

    setWindowTitle(tr("Shift Manager"));
    setMinimumSize(300, 200);
}

void ShiftManager::showEvent(QShowEvent *e)
{
    model->refreshData(true);
    QWidget::showEvent(e);
}

void ShiftManager::setReadOnly(bool readOnly)
{
    if (m_readOnly == readOnly)
        return;

    m_readOnly = readOnly;

    actionGroup->setEnabled(!m_readOnly);

    if (m_readOnly)
    {
        view->setEditTriggers(QTableView::NoEditTriggers);
    }
    else
    {
        view->setEditTriggers(QTableView::DoubleClicked);
    }
}

void ShiftManager::onNewShift()
{
    DEBUG_ENTRY;
    if (m_readOnly)
        return;

    OwningQPointer<QInputDialog> dlg = new QInputDialog(this);
    dlg->setWindowTitle(tr("Add Job Shift"));
    dlg->setLabelText(tr("Please choose a name for the new shift."));
    dlg->setTextValue(QString());

    do
    {
        int ret = dlg->exec();
        if (ret != QDialog::Accepted || !dlg)
        {
            break; // User canceled
        }

        const QString name = dlg->textValue().simplified();
        if (name.isEmpty())
        {
            QMessageBox::warning(this, tr("Error"), tr("Shift name cannot be empty."));
            continue; // Second chance
        }

        if (model->addShift(name))
        {
            break; // Done!
        }
    } while (true);
}

void ShiftManager::onRemoveShift()
{
    DEBUG_ENTRY;
    if (m_readOnly || !view->selectionModel()->hasSelection())
        return;

    QModelIndex idx = view->currentIndex();

    // Ask confirmation
    int ret = QMessageBox::question(this, tr("Remove Shift?"),
                                    tr("Are you sure you want to remove Job Shift <b>%1</b>?")
                                      .arg(model->shiftNameAtRow(idx.row())));
    if (ret != QMessageBox::Yes)
        return;

    model->removeShiftAt(idx.row());
}

void ShiftManager::onViewShift()
{
    DEBUG_ENTRY;
    if (!view->selectionModel()->hasSelection())
        return;

    db_id shiftId = model->shiftAtRow(view->currentIndex().row());
    if (!shiftId)
        return;

    qDebug() << "Display Shift:" << shiftId;

    Session->getViewManager()->requestShiftViewer(shiftId);
}

void ShiftManager::displayGraph()
{
    Session->getViewManager()->requestShiftGraphEditor();
}

void ShiftManager::onShiftSelectionChanged()
{
    const bool hasSel = view->selectionModel()->hasSelection();
    act_Remove->setEnabled(hasSel);
    act_displayShift->setEnabled(hasSel);
    act_Sheet->setEnabled(hasSel);
}

void ShiftManager::onModelError(const QString &msg)
{
    QMessageBox::warning(this, tr("Shift Error"), msg);
}

void ShiftManager::onSaveSheet()
{
    const QString shiftSheetDirKey = QLatin1String("shift_sheet_dir");

    DEBUG_ENTRY;
    if (!view->selectionModel()->hasSelection())
        return;

    QModelIndex idx = view->currentIndex();
    db_id shiftId   = model->shiftAtRow(idx.row());
    if (!shiftId)
        return;

    QString shiftName = model->shiftNameAtRow(idx.row());
    qDebug() << "Printing Shift:" << shiftId;

    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, tr("Save Shift Sheet"));
    dlg->setFileMode(QFileDialog::AnyFile);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDirectory(RecentDirStore::getDir(shiftSheetDirKey, RecentDirStore::Documents));
    dlg->selectFile(tr("shift_%1.odt").arg(shiftName));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::odtFormat);
    dlg->setNameFilters(filters);

    if (dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();
    if (fileName.isEmpty())
        return;

    RecentDirStore::setPath(shiftSheetDirKey, fileName);

    ShiftSheetExport w(Session->m_Db, shiftId);
    w.write();
    w.save(fileName);

    utils::OpenFileInFolderDlg::askUser(tr("Shift Sheet Saved"), fileName, this);
}
