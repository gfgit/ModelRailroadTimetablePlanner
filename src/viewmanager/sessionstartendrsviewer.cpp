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

#include "sessionstartendrsviewer.h"

#include "sessionstartendmodel.h"

#include <QVBoxLayout>
#include <QTreeView>
#include <QToolBar>
#include <QComboBox>

#include <QFileDialog>
#include "utils/files/recentdirstore.h"
#include "utils/owningqpointer.h"

#include "app/session.h"

#include "odt_export/sessionrsexport.h"
#include "utils/files/openfileinfolder.h"

#include "utils/files/file_format_names.h"

#include <QDebug>

SessionStartEndRSViewer::SessionStartEndRSViewer(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    QToolBar *toolBar = new QToolBar;
    lay->addWidget(toolBar);

    modeCombo = new QComboBox;
    modeCombo->addItem(tr("Show Session Start"), int(SessionRSMode::StartOfSession));
    modeCombo->addItem(tr("Show Session End"), int(SessionRSMode::EndOfSession));
    toolBar->addWidget(modeCombo);

    orderCombo = new QComboBox;
    orderCombo->addItem(tr("Order By Station"), int(SessionRSOrder::ByStation));
    orderCombo->addItem(tr("Order By Owner"), int(SessionRSOrder::ByOwner));
    toolBar->addWidget(orderCombo);

    connect(modeCombo, qOverload<int>(&QComboBox::activated), this, &SessionStartEndRSViewer::modeChanged);
    connect(orderCombo, qOverload<int>(&QComboBox::activated), this, &SessionStartEndRSViewer::orderChanged);

    toolBar->addAction(tr("Export Sheet"), this, &SessionStartEndRSViewer::exportSheet);

    view = new QTreeView(this);
    lay->addWidget(view);

    model = new SessionStartEndModel(Session->m_Db, this);

    view->setModel(model);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::SingleSelection);

    model->setMode(SessionRSMode::StartOfSession, SessionRSOrder::ByStation, true);
    modeCombo->setCurrentIndex(modeCombo->findData(int(model->mode())));
    orderCombo->setCurrentIndex(orderCombo->findData(int(model->order())));
    view->expandAll();

    setMinimumSize(200, 300);
    setWindowTitle(tr("Rollingstock Summary"));
}

void SessionStartEndRSViewer::orderChanged()
{
    SessionRSOrder order = SessionRSOrder(orderCombo->currentData().toInt());

    model->setMode(model->mode(), order, false);
    view->expandAll();
}

void SessionStartEndRSViewer::modeChanged()
{
    SessionRSMode mode = SessionRSMode(modeCombo->currentData().toInt());

    model->setMode(mode, model->order(), false);
    view->expandAll();
}

void SessionStartEndRSViewer::exportSheet()
{
    const QLatin1String session_rs_key = QLatin1String("session_rs_dir");

    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, tr("Expoert RS session plan"));
    dlg->setFileMode(QFileDialog::AnyFile);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setDirectory(RecentDirStore::getDir(session_rs_key, RecentDirStore::Documents));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::odtFormat);
    dlg->setNameFilters(filters);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();
    if(fileName.isEmpty())
        return;

    RecentDirStore::setPath(session_rs_key, fileName);

    SessionRSExport w(model->mode(), model->order());
    w.write();
    w.save(fileName);

    utils::OpenFileInFolderDlg::askUser(tr("Session RS Plan Saved"), fileName, this);
}
