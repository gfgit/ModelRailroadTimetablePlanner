#include "stationjobview.h"
#include "ui_stationjobview.h"

#include "utils/platform_utils.h"

#include "app/session.h"

#include "viewmanager/viewmanager.h"

#include "app/scopedebug.h"

#include <QTime>
#include <QMap>

#include <QMenu>

#include <QFileDialog>
#include <QStandardPaths>
#include "utils/owningqpointer.h"

#include "odt_export/stationsheetexport.h"

#include "stationplanmodel.h"

#include "utils/file_format_names.h"

StationJobView::StationJobView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StationJobView),
    m_stationId(0)
{
    ui->setupUi(this);

    model = new StationPlanModel(Session->m_Db, this);

    ui->tableView->setModel(model);

    ui->tableView->setColumnWidth(0, 60); //Arrival
    ui->tableView->setColumnWidth(1, 60); //Departure
    ui->tableView->setColumnWidth(2, 55); //Platform
    ui->tableView->setColumnWidth(3, 90); //Job
    ui->tableView->setColumnWidth(4, 115); //Notes
    ui->tableView->setSelectionBehavior(QTableView::SelectRows);

    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &StationJobView::showContextMenu);

    connect(ui->sheetButton, &QPushButton::pressed, this, &StationJobView::onSaveSheet);
    connect(ui->updateButt, &QPushButton::clicked, this, &StationJobView::updateInfo);

    setMinimumSize(300, 200);
    resize(450, 300);
}

StationJobView::~StationJobView()
{
    delete ui;
}

void StationJobView::setStation(db_id stId)
{
    m_stationId = stId;
}

void StationJobView::updateName()
{
    query q(Session->m_Db, "SELECT name FROM stations WHERE id=?");
    q.bind(1, m_stationId);
    q.step();
    setWindowTitle(q.getRows().get<QString>(0));
}

void StationJobView::updateInfo()
{
    updateName();
    updateJobsList();
}

void StationJobView::updateJobsList()
{
    model->loadPlan(m_stationId);
}

void StationJobView::showContextMenu(const QPoint& pos)
{
    DEBUG_ENTRY;

    QModelIndex idx = ui->tableView->indexAt(pos);
    if(!idx.isValid())
        return;

    std::pair<db_id, db_id> item = model->getJobAndStopId(idx.row());

    QMenu menu(this);

    QAction *showInJobEditor = new QAction(tr("Show in Job Editor"), &menu);
    QAction *selectJobInGraph = new QAction(tr("Show job in graph"), &menu);
    menu.addAction(showInJobEditor);
    menu.addAction(selectJobInGraph);

    QAction *act = menu.exec(ui->tableView->viewport()->mapToGlobal(pos));
    if(act == showInJobEditor)
    {
        Session->getViewManager()->requestJobEditor(item.first, item.second);
    }
    else if(act == selectJobInGraph)
    {
        Session->getViewManager()->requestJobSelection(item.first, true, true);
    }
}

void StationJobView::onSaveSheet()
{
    DEBUG_ENTRY;

    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, tr("Save Station Sheet"));
    dlg->setFileMode(QFileDialog::AnyFile);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    dlg->selectFile(tr("%1_station.odt").arg(windowTitle()));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::odtFormat);
    dlg->setNameFilters(filters);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();

    if(fileName.isEmpty())
        return;

    StationSheetExport sheet(m_stationId);
    sheet.write();
    sheet.save(fileName);
}
