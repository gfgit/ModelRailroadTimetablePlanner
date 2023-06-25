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

#include "splitrailwaysegmentdlg.h"

#include "../model/railwaysegmenthelper.h"
#include "../model/railwaysegmentsplithelper.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#include <QMessageBox>
#include <QDialogButtonBox>

#include "utils/delegates/sql/customcompletionlineedit.h"
#include "stations/match_models/railwaysegmentmatchmodel.h"
#include "stations/match_models/stationsmatchmodel.h"
#include "stations/match_models/stationgatesmatchmodel.h"
#include "stations/manager/segments/model/railwaysegmentconnectionsmodel.h"

#include "utils/owningqpointer.h"
#include "utils/delegates/sql/chooseitemdlg.h"
#include "editrailwaysegmentdlg.h"

SplitRailwaySegmentDlg::SplitRailwaySegmentDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    mDb(db)
{
    stationsModel      = new StationsMatchModel(mDb, this);
    middleInGateModel  = new StationGatesMatchModel(mDb, this);
    middleOutGateModel = new StationGatesMatchModel(mDb, this);
    stationsModel->setFilter(0);

    origSegConnModel = new RailwaySegmentConnectionsModel(mDb, this);
    newSegConnModel  = new RailwaySegmentConnectionsModel(mDb, this);

    QVBoxLayout *lay = new QVBoxLayout(this);

    // Segment:
    selectSegBut = new QPushButton("Select Segment");
    lay->addWidget(selectSegBut);

    segmentLabel = new QLabel;
    lay->addWidget(segmentLabel);

    // From:
    fromBox              = new QGroupBox(tr("From:"));
    QFormLayout *formLay = new QFormLayout(fromBox);

    fromStationLabel     = new QLabel;
    formLay->addRow(tr("Station:"), fromStationLabel);

    fromGateLabel = new QLabel;
    formLay->addRow(tr("Gate:"), fromGateLabel);
    lay->addWidget(fromBox);

    editOldSegBut = new QPushButton(tr("Edit First Segment"));
    lay->addWidget(editOldSegBut);

    // Middle:
    middleBox = new QGroupBox(tr("Middle:"));
    formLay   = new QFormLayout(middleBox);

    // Segment Out Gate is Middle Station In Gate
    middleInGateEdit = new CustomCompletionLineEdit(middleInGateModel);
    formLay->addRow(tr("In Gate:"), middleInGateEdit);

    middleStationEdit = new CustomCompletionLineEdit(stationsModel);
    formLay->addRow(tr("Station:"), middleStationEdit);

    // Middle Station Out Gate is New Segment In Gate
    middleOutGateEdit = new CustomCompletionLineEdit(middleOutGateModel);
    formLay->addRow(tr("Out Gate:"), middleOutGateEdit);
    lay->addWidget(middleBox);

    editNewSegBut = new QPushButton(tr("Edit Second Segment"));
    lay->addWidget(editNewSegBut);

    // To:
    toBox       = new QGroupBox(tr("To:"));
    formLay     = new QFormLayout(toBox);

    toGateLabel = new QLabel;
    formLay->addRow(tr("Gate:"), toGateLabel);

    toStationLabel = new QLabel;
    formLay->addRow(tr("Station:"), toStationLabel);
    lay->addWidget(toBox);

    butBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(butBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(butBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(butBox);

    connect(selectSegBut, &QPushButton::clicked, this, &SplitRailwaySegmentDlg::selectSegment);
    connect(editOldSegBut, &QPushButton::clicked, this, &SplitRailwaySegmentDlg::editOldSegment);
    connect(editNewSegBut, &QPushButton::clicked, this, &SplitRailwaySegmentDlg::editNewSegment);

    connect(middleStationEdit, &CustomCompletionLineEdit::completionDone, this,
            &SplitRailwaySegmentDlg::onStationSelected);
    connect(middleInGateEdit, &CustomCompletionLineEdit::completionDone, this,
            &SplitRailwaySegmentDlg::onMiddleInCompletionDone);
    connect(middleOutGateEdit, &CustomCompletionLineEdit::completionDone, this,
            &SplitRailwaySegmentDlg::onMiddleOutCompletionDone);

    // Reset segment
    setMainSegment(0);

    setMinimumSize(200, 200);
    resize(400, 500);
    setWindowTitle(tr("Split Segment"));
}

SplitRailwaySegmentDlg::~SplitRailwaySegmentDlg()
{
}

void SplitRailwaySegmentDlg::done(int res)
{
    if (res == QDialog::Accepted)
    {
        if (!origSegInfo.to.stationId)
        {
            QMessageBox::warning(this, tr("No Station"),
                                 tr("You must choose a station to put in the middle."));
            return;
        }

        if (!origSegInfo.to.gateId || !newSegInfo.from.gateId)
        {
            QMessageBox::warning(this, tr("No Gates"),
                                 tr("You must choose in and out gates for middle Station."));
            return;
        }

        RailwaySegmentSplitHelper helper(mDb, origSegConnModel, newSegConnModel);
        helper.setInfo(origSegInfo, newSegInfo);
        if (!helper.split())
        {
            QMessageBox::warning(this, tr("Error"), tr("Split of segment failed."));
            return;
        }
    }

    QDialog::done(res);
}

void SplitRailwaySegmentDlg::selectSegment()
{
    RailwaySegmentMatchModel segmentMatch(mDb);
    segmentMatch.setFilter(0, 0, 0);
    OwningQPointer<ChooseItemDlg> dlg = new ChooseItemDlg(&segmentMatch, this);
    dlg->setDescription(tr("Choose a Railway Segment to split in 2 parts.\n"
                           "A station will be inserted in the middle."));
    dlg->setPlaceholder(tr("Segment..."));
    if (dlg->exec() != QDialog::Accepted || !dlg)
        return;

    const db_id segmentId = dlg->getItemId();
    setMainSegment(segmentId);
}

void SplitRailwaySegmentDlg::onStationSelected()
{
    db_id stationId;
    QString tmp;
    middleStationEdit->getData(stationId, tmp);

    setMiddleStation(stationId);
}

void SplitRailwaySegmentDlg::editOldSegment()
{
    OwningQPointer<EditRailwaySegmentDlg> dlg =
      new EditRailwaySegmentDlg(mDb, origSegConnModel, this);
    dlg->setManuallyApply(true);
    dlg->setSegmentInfo(origSegInfo);
    if (dlg->exec() != QDialog::Accepted || !dlg)
        return;

    // Get info back
    dlg->fillSegInfo(origSegInfo);
}

void SplitRailwaySegmentDlg::editNewSegment()
{
    OwningQPointer<EditRailwaySegmentDlg> dlg =
      new EditRailwaySegmentDlg(mDb, newSegConnModel, this);
    dlg->setManuallyApply(true);
    dlg->setSegmentInfo(newSegInfo);
    if (dlg->exec() != QDialog::Accepted || !dlg)
        return;

    // Get info back
    dlg->fillSegInfo(newSegInfo);
}

void SplitRailwaySegmentDlg::onMiddleInCompletionDone()
{
    QString tmp;
    middleInGateEdit->getData(origSegInfo.to.gateId, tmp);
    if (tmp.size())
        origSegInfo.to.gateLetter = tmp.front();
}

void SplitRailwaySegmentDlg::onMiddleOutCompletionDone()
{
    QString tmp;
    middleOutGateEdit->getData(newSegInfo.from.gateId, tmp);
    if (tmp.size())
        newSegInfo.from.gateLetter = tmp.front();
}

void SplitRailwaySegmentDlg::setMainSegment(db_id segmentId)
{
    origSegInfo.segmentId = segmentId;

    // Reset outer info
    origSegInfo.from = newSegInfo.to = utils::RailwaySegmentGateInfo();

    utils::RailwaySegmentInfo info;
    info.segmentId = origSegInfo.segmentId;
    RailwaySegmentHelper helper(mDb);

    if (origSegInfo.segmentId && helper.getSegmentInfo(info))
    {
        origSegInfo.segmentName    = info.segmentName;
        origSegInfo.distanceMeters = info.distanceMeters;
        origSegInfo.maxSpeedKmH    = info.maxSpeedKmH;
        origSegInfo.type           = info.type;

        // Split segment info in 2 parts
        origSegInfo.from = info.from;
        newSegInfo.to    = info.to;

        // Get station names
        origSegInfo.from.stationName = stationsModel->getName(origSegInfo.from.stationId);
        newSegInfo.to.stationName    = stationsModel->getName(newSegInfo.to.stationId);

        // Get gate names
        QString tmp = middleInGateModel->getName(origSegInfo.from.gateId);
        if (tmp.size())
            origSegInfo.from.gateLetter = tmp.front();

        tmp = middleInGateModel->getName(newSegInfo.to.gateId);
        if (tmp.size())
            newSegInfo.to.gateLetter = tmp.front();

        newSegInfo.type = origSegInfo.type;
    }
    else
    {
        // Failed to reteive info
        origSegInfo.segmentId = 0;
    }

    segmentLabel->setText(tr("Segment: <b>%1</b>").arg(origSegInfo.segmentName));

    fromStationLabel->setText(origSegInfo.from.stationName);
    fromGateLabel->setText(origSegInfo.from.gateLetter);

    toStationLabel->setText(newSegInfo.to.stationName);
    toGateLabel->setText(newSegInfo.to.gateLetter);

    // Allow setting middle station only after settin main segment
    middleStationEdit->setEnabled(origSegInfo.segmentId != 0);

    butBox->button(QDialogButtonBox::Ok)->setEnabled(origSegInfo.segmentId != 0);

    // Reset middle station
    setMiddleStation(0);
}

void SplitRailwaySegmentDlg::setMiddleStation(db_id stationId)
{
    // Reset station
    origSegInfo.to = newSegInfo.from = utils::RailwaySegmentGateInfo();

    origSegInfo.to.stationId = newSegInfo.from.stationId = stationId;

    const QString middleStName                           = stationsModel->getName(stationId);
    origSegInfo.to.stationName = newSegInfo.from.stationName = middleStName;

    if (middleStName.isEmpty())
        newSegInfo.segmentName.clear();
    else
        setNewSegmentName(newSegInfo.from.stationName + '-' + newSegInfo.to.stationName);

    middleStationEdit->setData(stationId, middleStName);
    middleInGateEdit->setData(0);
    middleOutGateEdit->setData(0);

    middleInGateModel->setFilter(stationId, true, 0);
    middleOutGateModel->setFilter(stationId, true, 0);

    // Allow selecting Gates and Edit Segment only after Middle Station is set
    middleInGateEdit->setEnabled(stationId != 0);
    middleOutGateEdit->setEnabled(stationId != 0);
    editOldSegBut->setEnabled(stationId != 0);
    editNewSegBut->setEnabled(stationId != 0);
}

void SplitRailwaySegmentDlg::setNewSegmentName(const QString &possibleName)
{
    // Find available segment name
    query q(mDb, "SELECT id FROM railway_segments WHERE name=? LIMIT 1");
    const QString fmt = QLatin1String("%1_%2");
    QString name      = possibleName;
    for (int i = 0; i < 10; i++)
    {
        if (i != 0)
            name = fmt.arg(possibleName).arg(i);

        q.bind(1, name);
        int ret = q.step();
        if (ret == SQLITE_OK || ret == SQLITE_DONE)
        {
            // Found available name
            newSegInfo.segmentName = name;
            break;
        }
        q.reset();
    }
}
