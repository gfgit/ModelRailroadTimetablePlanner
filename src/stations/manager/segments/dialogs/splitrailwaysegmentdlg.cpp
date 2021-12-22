#include "splitrailwaysegmentdlg.h"

#include "../model/railwaysegmenthelper.h"
#include "../model/railwaysegmentsplithelper.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#include "utils/sqldelegate/customcompletionlineedit.h"
#include "stations/match_models/railwaysegmentmatchmodel.h"
#include "stations/match_models/stationsmatchmodel.h"
#include "stations/match_models/stationgatesmatchmodel.h"

#include "utils/owningqpointer.h"
#include "utils/sqldelegate/chooseitemdlg.h"
#include "editrailwaysegmentdlg.h"

SplitRailwaySegmentDlg::SplitRailwaySegmentDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    mDb(db)
{
    stationsModel = new StationsMatchModel(mDb, this);
    middleInGateModel = new StationGatesMatchModel(mDb, this);
    middleOutGateModel = new StationGatesMatchModel(mDb, this);

    QVBoxLayout *lay = new QVBoxLayout(this);

    //Segment:
    selectSegBut = new QPushButton("Select Segment");
    lay->addWidget(selectSegBut);

    segmentLabel = new QLabel;
    lay->addWidget(segmentLabel);

    //From:
    fromBox = new QGroupBox(tr("From:"));
    QFormLayout *formLay = new QFormLayout(fromBox);

    fromStationLabel = new QLabel;
    formLay->addRow(tr("Station:"), fromStationLabel);

    fromGateLabel = new QLabel;
    formLay->addRow(tr("Gate:"), fromGateLabel);
    lay->addWidget(fromBox);

    //Middle:
    middleBox = new QGroupBox(tr("Middle:"));
    formLay = new QFormLayout(middleBox);

    //Segment Out Gate is Middle Station In Gate
    middleOutGateEdit = new CustomCompletionLineEdit(middleInGateModel);
    formLay->addRow(tr("In Gate:"), middleOutGateEdit);

    middleStationEdit = new CustomCompletionLineEdit(stationsModel);
    formLay->addRow(tr("Station:"), middleStationEdit);

    //Middle Station Out Gate is New Segment In Gate
    middleOutGateEdit = new CustomCompletionLineEdit(middleOutGateModel);
    formLay->addRow(tr("Out Gate:"), middleOutGateEdit);
    lay->addWidget(middleBox);

    //To:
    toBox = new QGroupBox(tr("To:"));
    formLay = new QFormLayout(toBox);

    toGateLabel = new QLabel;
    formLay->addRow(tr("Gate:"), toGateLabel);

    toStationLabel = new QLabel;
    formLay->addRow(tr("Station:"), toStationLabel);
    lay->addWidget(toBox);

    connect(selectSegBut, &QPushButton::clicked, this, &SplitRailwaySegmentDlg::selectSegment);
    connect(middleStationEdit, &CustomCompletionLineEdit::completionDone,
            this, &SplitRailwaySegmentDlg::onStationSelected);

    //Reset segment
    setMainSegment(0);
}

void SplitRailwaySegmentDlg::selectSegment()
{
    RailwaySegmentMatchModel segmentMatch(mDb);
    OwningQPointer<ChooseItemDlg> dlg = new ChooseItemDlg(&segmentMatch, this);
    dlg->setDescription(tr("Choose a Railway Segment to split in 2 parts.\n"
                           "A station will be inserted in the middle."));
    dlg->setPlaceholder(tr("Segment..."));
    if(dlg->exec() != QDialog::Accepted || !dlg)
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

void SplitRailwaySegmentDlg::setMainSegment(db_id segmentId)
{
    mOriginalSegmentId = segmentId;

    //Reset info
    fromGate = Gate();
    toGate = Gate();

    QString fromStationName, toStationName;
    RailwaySegmentInfo info;
    RailwaySegmentHelper helper(mDb);

    if(mOriginalSegmentId && helper.getSegmentInfo(info))
    {
        fromGate.gateId = info.from.gateId;
        fromGate.stationId = info.from.stationId;
        QString tmp = middleInGateModel->getName(fromGate.gateId);
        if(tmp.size())
            fromGate.gateLetter = tmp.front();
        fromStationName = stationsModel->getName(fromGate.stationId);

        toGate.gateId = info.to.gateId;
        toGate.stationId = info.to.stationId;
        tmp = middleInGateModel->getName(toGate.gateId);
        if(tmp.size())
            toGate.gateLetter = tmp.front();
        toStationName = stationsModel->getName(toGate.stationId);

        segmentType = info.type;
    }

    segmentLabel->setText(tr("Segment: <b>%1</b>").arg(info.segmentName));

    fromStationLabel->setText(fromStationName);
    fromGateLabel->setText(fromGate.gateLetter);

    toStationLabel->setText(toStationName);
    toGateLabel->setText(toGate.gateLetter);

    //Reset middle station
    setMiddleStation(0);

    //Allow setting middle station only after settin main segment
    middleStationEdit->setEnabled(mOriginalSegmentId != 0);
}

void SplitRailwaySegmentDlg::setMiddleStation(db_id stationId)
{
    //Reset station
    middleInGate = Gate();
    middleOutGate = Gate();

    middleInGate.stationId = middleOutGate.stationId = stationId;

    middleStationEdit->setData(stationId);
    middleInGateEdit->setData(0);
    middleOutGateEdit->setData(0);

    //Allow selectiig Gates only after Station
    middleInGateEdit->setEnabled(stationId != 0);
    middleOutGateEdit->setEnabled(stationId != 0);
}
