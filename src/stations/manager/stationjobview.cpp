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

#include "stationjobview.h"
#include "ui_stationjobview.h"

#include "app/session.h"

#include "viewmanager/viewmanager.h"

#include "app/scopedebug.h"

#include "utils/owningqpointer.h"
#include <QFileDialog>
#include "utils/files/recentdirstore.h"
#include <QMenu>

#include "odt_export/stationsheetexport.h"
#include "utils/files/openfileinfolder.h"

#include "stationplanmodel.h"

#include "utils/files/file_format_names.h"

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

    StPlanItem item = model->getItemAt(idx.row());

    OwningQPointer<QMenu> menu = new QMenu(this);

    QAction *showInJobEditor = new QAction(tr("Show in Job Editor"), menu);
    QAction *selectJobInGraph = new QAction(tr("Show job in graph"), menu);
    QAction *showSVGPlan = new QAction(tr("Show Station SVG Plan"), menu);
    menu->addAction(showInJobEditor);
    menu->addAction(selectJobInGraph);
    menu->addAction(showSVGPlan);

    QAction *act = menu->exec(ui->tableView->viewport()->mapToGlobal(pos));
    if(act == showInJobEditor)
    {
        Session->getViewManager()->requestJobEditor(item.jobId, item.stopId);
    }
    else if(act == selectJobInGraph)
    {
        Session->getViewManager()->requestJobSelection(item.jobId, true, true);
    }
    else if(act == showSVGPlan)
    {
        Session->getViewManager()->requestStSVGPlan(m_stationId, true, item.arrival);
    }
}

void StationJobView::onSaveSheet()
{
    DEBUG_ENTRY;

    const QLatin1String station_sheet_key = QLatin1String("station_sheet_dir");

    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, tr("Save Station Sheet"));
    dlg->setFileMode(QFileDialog::AnyFile);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDirectory(RecentDirStore::getDir(station_sheet_key, RecentDirStore::Documents));
    dlg->selectFile(tr("%1_station.odt").arg(windowTitle()));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::odtFormat);
    dlg->setNameFilters(filters);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();

    if(fileName.isEmpty())
        return;

    RecentDirStore::setPath(station_sheet_key, fileName);

    StationSheetExport sheet(m_stationId);
    sheet.write();
    sheet.save(fileName);

    utils::OpenFileInFolderDlg::askUser(tr("Station Sheet Saved"), fileName, this);
}
