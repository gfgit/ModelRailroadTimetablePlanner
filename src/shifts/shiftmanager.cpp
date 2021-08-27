#include "shiftmanager.h"

#include "shiftsqlmodel.h"
#include "utils/sqldelegate/modelpageswitcher.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include "app/scopedebug.h"

#include <QVBoxLayout>

#include <QTableView>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>

#include <QToolBar>
#include <QActionGroup>

#include "odt_export/shiftsheetexport.h"

#include "utils/file_format_names.h"

ShiftManager::ShiftManager(QWidget *parent) :
    QWidget(parent),
    m_readOnly(false),
    edited(false)
{
    QVBoxLayout *l = new QVBoxLayout(this);

    toolBar = new QToolBar(this);
    l->addWidget(toolBar);

    view = new QTableView(this);
    l->addWidget(view);

    auto ps = new ModelPageSwitcher(false, this);
    l->addWidget(ps);

    QFont f = view->font();
    f.setPointSize(12);
    view->setFont(f);

    model = new ShiftSQLModel(Session->m_Db, this);
    model->refreshData();
    ps->setModel(model);
    view->setModel(model);

    act_New    = toolBar->addAction(tr("New"), this, &ShiftManager::onNewShift);
    act_Remove = toolBar->addAction(tr("Remove"), this, &ShiftManager::onRemoveShift);
    act_displayShift   = toolBar->addAction(tr("View Shift"), this, &ShiftManager::onViewShift);
    act_Sheet  = toolBar->addAction(tr("Sheet"), this, &ShiftManager::onSaveSheet);
    act_Graph  = toolBar->addAction(tr("Graph"), this, &ShiftManager::displayGraph);

    actionGroup = new QActionGroup(this);
    actionGroup->addAction(act_New);
    actionGroup->addAction(act_Remove);

    setReadOnly(false);

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

    QModelIndex index = model->index(row, ShiftSQLModel::ShiftName);
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

    QFileDialog dlg(this, tr("Save Shift Sheet"));
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    dlg.selectFile(tr("shift_%1.odt").arg(shiftName));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::odtFormat);
    dlg.setNameFilters(filters);

    if(dlg.exec() != QDialog::Accepted)
        return;

    QString fileName = dlg.selectedUrls().value(0).toLocalFile();

    if(fileName.isEmpty())
        return;

    ShiftSheetExport w(shiftId);
    w.write();
    w.save(fileName);
}
