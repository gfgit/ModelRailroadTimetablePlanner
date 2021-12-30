#include "shiftmanager.h"

#include "shiftsmodel.h"
#include "utils/delegates/sql/modelpageswitcher.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include "app/scopedebug.h"

#include <QVBoxLayout>

#include <QTableView>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include "utils/owningqpointer.h"

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

    toolBar = new QToolBar(this);
    l->addWidget(toolBar);

    view = new QTableView(this);
    view->setSelectionMode(QTableView::SingleSelection);
    l->addWidget(view);

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
    act_displayShift   = toolBar->addAction(tr("View Shift"), this, &ShiftManager::onViewShift);
    act_Sheet  = toolBar->addAction(tr("Sheet"), this, &ShiftManager::onSaveSheet);
    toolBar->addSeparator();
    act_Graph  = toolBar->addAction(tr("Graph"), this, &ShiftManager::displayGraph);

    actionGroup = new QActionGroup(this);
    actionGroup->addAction(act_New);
    actionGroup->addAction(act_Remove);

    connect(view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ShiftManager::onShiftSelectionChanged);
    connect(model, &QAbstractItemModel::modelReset,
            this, &ShiftManager::onShiftSelectionChanged);

    setReadOnly(false);

    //Action Tooltips
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
    if(m_readOnly == readOnly)
        return;

    m_readOnly = readOnly;

    actionGroup->setEnabled(!m_readOnly);

    if(m_readOnly)
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
    if(m_readOnly)
        return;

    int row = 0;
    if(!model->addShift(&row) || row == -1)
    {
        QMessageBox::warning(this,
                             tr("Error Adding Shift"),
                             tr("An error occurred while adding a new shift:\n%1")
                             .arg(Session->m_Db.error_msg()));
        return;
    }

    QModelIndex index = model->index(row, ShiftsModel::ShiftName);
    view->setCurrentIndex(index);
    view->scrollTo(index);
    view->edit(index); //FIXME: item is not yet fetched so editing fails, maybe queue edit?
}

void ShiftManager::onRemoveShift()
{
    DEBUG_ENTRY;
    if(m_readOnly || !view->selectionModel()->hasSelection())
        return;

    QModelIndex idx = view->currentIndex();

    //Ask confirmation
    int ret = QMessageBox::question(this, tr("Remove Shift?"),
                                    tr("Are you sure you want to remove Job Shift <b>%1</b>?")
                                        .arg(model->shiftNameAtRow(idx.row())));
    if(ret != QMessageBox::Yes)
        return;

    model->removeShiftAt(idx.row());
}

void ShiftManager::onViewShift()
{
    DEBUG_ENTRY;
    if(!view->selectionModel()->hasSelection())
        return;

    db_id shiftId = model->shiftAtRow(view->currentIndex().row());
    if(!shiftId)
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

void ShiftManager::onSaveSheet()
{
    DEBUG_ENTRY;
    if(!view->selectionModel()->hasSelection())
        return;

    QModelIndex idx = view->currentIndex();
    db_id shiftId = model->shiftAtRow(idx.row());
    if(!shiftId)
        return;

    QString shiftName = model->shiftNameAtRow(idx.row());
    qDebug() << "Printing Shift:" << shiftId;

    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, tr("Save Shift Sheet"));
    dlg->setFileMode(QFileDialog::AnyFile);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    dlg->selectFile(tr("shift_%1.odt").arg(shiftName));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::odtFormat);
    dlg->setNameFilters(filters);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();
    if(fileName.isEmpty())
        return;

    ShiftSheetExport w(Session->m_Db, shiftId);
    w.write();
    w.save(fileName);

    utils::OpenFileInFolderDlg::askUser(tr("Shift Sheet Saved"), fileName, this);
}
