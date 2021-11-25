#include "stopeditor.h"

#include "utils/sqldelegate/customcompletionlineedit.h"

#include "stations/match_models/stationsmatchmodel.h"
#include "stations/match_models/stationtracksmatchmodel.h"
#include "stations/match_models/railwaysegmentmatchmodel.h"

#include <QTimeEdit>
#include <QAbstractItemView>
#include <QGridLayout>
#include <QMenu>

#include <QMouseEvent>

#include <QMessageBox>

#include <QGuiApplication>

StopEditor::StopEditor(sqlite3pp::database &db, StopModel *m, QWidget *parent) :
    QFrame(parent),
    model(m)
{
    stationsMatchModel = new StationsMatchModel(db, this);
    stationTrackMatchModel = new StationTracksMatchModel(db, this);
    segmentMatchModel = new RailwaySegmentMatchModel(db, this);

    mStationEdit = new CustomCompletionLineEdit(stationsMatchModel, this);
    mStationEdit->setPlaceholderText(tr("Station name"));
    connect(mStationEdit, &CustomCompletionLineEdit::completionDone, this, &StopEditor::onStationSelected);

    mTrackEdit = new CustomCompletionLineEdit(stationTrackMatchModel, this);
    mTrackEdit->setPlaceholderText(tr("Track"));
    connect(mTrackEdit, &CustomCompletionLineEdit::completionDone, this, &StopEditor::onTrackSelected);

    mSegmentEdit = new CustomCompletionLineEdit(segmentMatchModel, this);
    mSegmentEdit->setPlaceholderText(tr("Next segment"));
    connect(mSegmentEdit, &CustomCompletionLineEdit::completionDone, this, &StopEditor::onNextSegmentSelected);

    arrEdit = new QTimeEdit;
    depEdit = new QTimeEdit;
    connect(arrEdit, &QTimeEdit::timeChanged, this, &StopEditor::arrivalChanged);

#ifdef PRINT_DBG_MSG
    setObjectName(QStringLiteral("StopEditor (%1)").arg(qintptr(this)));
#endif

    setFrameShape(QFrame::Box);

    lay = new QGridLayout(this);
    lay->addWidget(mStationEdit, 0, 0);
    lay->addWidget(arrEdit, 0, 1);
    lay->addWidget(depEdit, 0, 2);
    lay->addWidget(mTrackEdit, 1, 0, 1, 3);
    lay->addWidget(mSegmentEdit, 2, 0, 1, 3);

    setTabOrder(mStationEdit, arrEdit);
    setTabOrder(arrEdit, depEdit);
    setTabOrder(depEdit, mSegmentEdit);
}

void StopEditor::setStop(const StopItem &item, const StopItem &prev)
{
    oldItem = item;
    prevItem = prev;

    arrEdit->setToolTip(QString());
    switch (item.type)
    {
    case Normal:
    {
        arrEdit->setToolTip(tr("Press shift if you don't want to change also departure time."));
        arrEdit->setEnabled(true);
        depEdit->setEnabled(true);
        break;
    }
    case Transit:
    case TransitLineChange:
    {
        arrEdit->setEnabled(true);

        depEdit->setEnabled(false);
        depEdit->setVisible(false);
        break;
    }
    case First:
    {
        arrEdit->setEnabled(false);
        arrEdit->setVisible(false);
        break;
    }
    case Last:
    {
        depEdit->setEnabled(false);
        depEdit->setVisible(false);

        mSegmentEdit->hide();
        if(item.stationId == 0)
            setFocusProxy(mStationEdit);
        break;
    }
    default:
        break;
    }

    if(item.type == First)
        stationsMatchModel->setFilter(0);
    else
        stationsMatchModel->setFilter(prevItem.stationId);
    mStationEdit->setData(item.stationId);

    stationTrackMatchModel->setFilter(item.stationId);
    mTrackEdit->setEnabled(item.stationId != 0); //Enable only if station is selected

    segmentMatchModel->setFilter(item.stationId, 0, 0);
    mSegmentEdit->setData(item.nextSegment.segmentId);

    //Set Arrival and Departure
    arrEdit->blockSignals(true);
    arrEdit->setTime(item.arrival);
    arrEdit->blockSignals(false);

    depEdit->setTime(item.departure);

    if(item.type != First)
    {
        /* Next stop must be at least one minute after
         * This is to prevent contemporary stops that will break ORDER BY arrival queries */
        const QTime minArr = prevItem.departure.addSecs(60);
        arrEdit->blockSignals(true);
        arrEdit->setMinimumTime(minArr);
        arrEdit->blockSignals(false);

        //First stop: arrival is hidden, you can change only departure so do not set a minimum
        //Normal stop: at least 1 minute stop
        //Transit, Last: departure = arrival
        QTime minDep = arrEdit->time();
        if(item.type == Normal)
            depEdit->setMinimumTime(minDep.addSecs(60));
        else
            depEdit->setMinimumTime(minDep);
    }
}

void StopEditor::onStationSelected()
{
    db_id newStId = 0;
    QString tmp;
    if(!mStationEdit->getData(newStId, tmp))
        return;

    if(newStId == oldItem.stationId)
        return;

    oldItem.stationId = newStId;

    //Update track
    stationTrackMatchModel->setFilter(oldItem.stationId);
    mTrackEdit->setEnabled(oldItem.stationId != 0); //Enable only if station is selected

    if(oldItem.stationId)
    {
        if(!model->trySelectTrackForStop(oldItem))
            oldItem.trackId = 0; //Could not find a track

        mTrackEdit->setData(oldItem.trackId);
    }

    //Update prev segment
    prevItem.nextSegment.segConnId = 0; //Reset, will be reloaded by model
    prevItem.nextSegment.inTrackNum = -1;
    prevItem.nextSegment.outTrackNum = -1;
    prevItem.nextSegment.segmentId = 0;

    //Update next segment
    segmentMatchModel->setFilter(oldItem.stationId, 0, 0);
    mSegmentEdit->setData(0); //Reset, user must choose again

    oldItem.nextSegment.segConnId = 0;
    oldItem.nextSegment.segmentId = 0;
    oldItem.nextSegment.inTrackNum = -1;
    oldItem.nextSegment.outTrackNum = -1;
}

void StopEditor::onTrackSelected()
{
    db_id newTrackId = 0;
    QString tmp;
    if(!mTrackEdit->getData(newTrackId, tmp))
        return;

    //Check if track is connected to gates
    if(!model->trySetTrackConnections(oldItem, newTrackId, &tmp))
    {
        //Show error to the user
        bool stillSucceded = (oldItem.trackId == newTrackId);
        QMessageBox::warning(this, stillSucceded ? tr("Gate Warning") : tr("Track Error"), tmp);

        if(!stillSucceded)
            mTrackEdit->setData(oldItem.trackId); //Reset to previous track
    }
}

void StopEditor::onNextSegmentSelected()
{
    db_id newSegId = 0;
    QString segmentName;
    if(!mSegmentEdit->getData(newSegId, segmentName))
        return;

    db_id outGateId = 0;
    if(!model->trySelectNextSegment(oldItem, newSegId, 0, outGateId))
    {
        QMessageBox::warning(this, tr("Stop Error"), tr("Cannot set segment '%1'").arg(segmentName));
    }
}

void StopEditor::arrivalChanged(const QTime& arrival)
{
    bool shiftPressed = QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
    QTime dep = depEdit->time();
    if(!shiftPressed)
    {
        //Shift departure by the same amount if SHIFT NOT pressed
        int diff = oldItem.arrival.msecsTo(arrival);
        dep = dep.addMSecs(diff);
    }
    QTime minDep = arrival;
    if(oldItem.type == Normal)
    {
        minDep = arrival.addSecs(60); //At least stop for 1 minute in Normal stops
    }
    depEdit->setMinimumTime(minDep);
    depEdit->setTime(dep); //Set after setting minimum time
    oldItem.arrival = arrival;
}
