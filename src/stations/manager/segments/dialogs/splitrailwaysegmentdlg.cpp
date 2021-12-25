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
    stationsModel->setFilter(0);

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
    middleInGateEdit = new CustomCompletionLineEdit(middleInGateModel);
    formLay->addRow(tr("In Gate:"), middleInGateEdit);

    middleStationEdit = new CustomCompletionLineEdit(stationsModel);
    formLay->addRow(tr("Station:"), middleStationEdit);

    //Middle Station Out Gate is New Segment In Gate
    middleOutGateEdit = new CustomCompletionLineEdit(middleOutGateModel);
    formLay->addRow(tr("Out Gate:"), middleOutGateEdit);

    editNewSegBut = new QPushButton(tr("Edit New Segment"));
    formLay->addWidget(editNewSegBut);
    lay->addWidget(middleBox);

    //To:
    toBox = new QGroupBox(tr("To:"));
    formLay = new QFormLayout(toBox);

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
    connect(editNewSegBut, &QPushButton::clicked, this, &SplitRailwaySegmentDlg::editNewSegment);

    connect(middleStationEdit, &CustomCompletionLineEdit::completionDone,
            this, &SplitRailwaySegmentDlg::onStationSelected);
    connect(middleInGateEdit, &CustomCompletionLineEdit::completionDone,
            this, &SplitRailwaySegmentDlg::onMiddleInCompletionDone);
    connect(middleOutGateEdit, &CustomCompletionLineEdit::completionDone,
            this, &SplitRailwaySegmentDlg::onMiddleOutCompletionDone);

    //Reset segment
    setMainSegment(0);

    setMinimumSize(200, 200);
    resize(400, 450);
    setWindowTitle(tr("Split Segment"));
}

void SplitRailwaySegmentDlg::done(int res)
{
    if(res == QDialog::Accepted)
    {
        if(!middleInGate.stationId)
        {
            QMessageBox::warning(this, tr("No Station"),
                                 tr("You must choose a station to put in the middle."));
            return;
        }

        if(!middleInGate.gateId || !newSegInfo.from.gateId)
        {
            QMessageBox::warning(this, tr("No Gates"),
                                 tr("You must choose in and out gates for middle Station."));
            return;
        }

        RailwaySegmentSplitHelper helper(mDb, mOriginalSegmentId);
        helper.setInfo(newSegInfo, middleInGate.gateId);
        if(!helper.split())
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Split of segment failed."));
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

void SplitRailwaySegmentDlg::editNewSegment()
{
    OwningQPointer<EditRailwaySegmentDlg> dlg = new EditRailwaySegmentDlg(mDb, this);
    dlg->setManuallyApply(true);
    dlg->setSegmentInfo(newSegInfo);
    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    //Get info back
    dlg->fillSegInfo(newSegInfo);
}

void SplitRailwaySegmentDlg::onMiddleInCompletionDone()
{
    QString tmp;
    middleInGateEdit->getData(middleInGate.gateId, tmp);
    if(tmp.size())
        middleInGate.gateLetter = tmp.front();
}

void SplitRailwaySegmentDlg::onMiddleOutCompletionDone()
{
    QString tmp;
    middleOutGateEdit->getData(newSegInfo.from.gateId, tmp);
    if(tmp.size())
        newSegInfo.from.gateLetter = tmp.front();
}

void SplitRailwaySegmentDlg::setMainSegment(db_id segmentId)
{
    mOriginalSegmentId = segmentId;

    //Reset info
    fromGate = newSegInfo.to = utils::RailwaySegmentGateInfo();

    utils::RailwaySegmentInfo info;
    info.segmentId = mOriginalSegmentId;
    RailwaySegmentHelper helper(mDb);

    if(mOriginalSegmentId && helper.getSegmentInfo(info))
    {
        fromGate.gateId = info.from.gateId;
        fromGate.stationId = info.from.stationId;
        QString tmp = middleInGateModel->getName(fromGate.gateId);
        if(tmp.size())
            fromGate.gateLetter = tmp.front();
        fromGate.stationName = stationsModel->getName(fromGate.stationId);

        newSegInfo.to.gateId = info.to.gateId;
        newSegInfo.to.stationId = info.to.stationId;
        tmp = middleInGateModel->getName(newSegInfo.to.gateId);
        if(tmp.size())
            newSegInfo.to.gateLetter = tmp.front();
        newSegInfo.to.stationName = stationsModel->getName(newSegInfo.to.stationId);

        newSegInfo.type = info.type;
    }
    else
    {
        //Failed to reteive info
        mOriginalSegmentId = 0;
    }

    segmentLabel->setText(tr("Segment: <b>%1</b>").arg(info.segmentName));

    fromStationLabel->setText(fromGate.stationName);
    fromGateLabel->setText(fromGate.gateLetter);

    toStationLabel->setText(newSegInfo.to.stationName);
    toGateLabel->setText(newSegInfo.to.gateLetter);

    //Allow setting middle station only after settin main segment
    middleStationEdit->setEnabled(mOriginalSegmentId != 0);

    butBox->button(QDialogButtonBox::Ok)->setEnabled(mOriginalSegmentId != 0);

    //Reset middle station
    setMiddleStation(0);
}

void SplitRailwaySegmentDlg::setMiddleStation(db_id stationId)
{
    //Reset station
    middleInGate = utils::RailwaySegmentGateInfo();
    newSegInfo.from = utils::RailwaySegmentGateInfo();

    newSegInfo.from.stationId = middleInGate.stationId = stationId;

    const QString middleStName = stationsModel->getName(stationId);
    newSegInfo.from.stationName = middleInGate.stationName = middleStName;

    if(middleStName.isEmpty())
        newSegInfo.segmentName.clear();
    else
        setNewSegmentName(newSegInfo.from.stationName + '-' + newSegInfo.to.stationName);


    middleStationEdit->setData(stationId, middleStName);
    middleInGateEdit->setData(0);
    middleOutGateEdit->setData(0);

    middleInGateModel->setFilter(stationId, true, 0);
    middleOutGateModel->setFilter(stationId, true, 0);

    //Allow selectiig Gates and Edit New Segment only after Middle Station is set
    middleInGateEdit->setEnabled(stationId != 0);
    middleOutGateEdit->setEnabled(stationId != 0);
    editNewSegBut->setEnabled(stationId != 0);
}

void SplitRailwaySegmentDlg::setNewSegmentName(const QString &possibleName)
{
    //Find available segment name
    query q(mDb, "SELECT id FROM railway_segments WHERE name=? LIMIT 1");
    const QString fmt = QLatin1String("%1_%2");
    QString name = possibleName;
    for(int i = 0; i < 10; i++)
    {
        if(i != 0)
            name = fmt.arg(possibleName).arg(i);

        q.bind(1, name);
        int ret = q.step();
        if(ret == SQLITE_OK || ret == SQLITE_DONE)
        {
            //Found available name
            newSegInfo.segmentName = name;
            break;
        }
        q.reset();
    }
}
