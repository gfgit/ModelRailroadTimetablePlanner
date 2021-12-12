#include "stopeditor.h"

#include "utils/sqldelegate/customcompletionlineedit.h"

#include "stations/match_models/stationsmatchmodel.h"
#include "stations/match_models/stationtracksmatchmodel.h"
#include "stations/match_models/stationgatesmatchmodel.h"

#include <QTimeEdit>
#include <QSpinBox>
#include <QAbstractItemView>
#include <QGridLayout>
#include <QMenu>

#include <QMouseEvent>

#include <QMessageBox>

#include <QGuiApplication>

StopEditor::StopEditor(sqlite3pp::database &db, StopModel *m, QWidget *parent) :
    QFrame(parent),
    model(m),
    mTimerOutTrack(0),
    m_closeOnSegmentChosen(false)
{
    stationsMatchModel = new StationsMatchModel(db, this);
    stationTrackMatchModel = new StationTracksMatchModel(db, this);
    stationOutGateMatchModel = new StationGatesMatchModel(db, this);

    mStationEdit = new CustomCompletionLineEdit(stationsMatchModel, this);
    mStationEdit->setPlaceholderText(tr("Station name"));
    connect(mStationEdit, &CustomCompletionLineEdit::completionDone, this, &StopEditor::onStationSelected);

    mStTrackEdit = new CustomCompletionLineEdit(stationTrackMatchModel, this);
    mStTrackEdit->setPlaceholderText(tr("Track"));
    connect(mStTrackEdit, &CustomCompletionLineEdit::completionDone, this, &StopEditor::onTrackSelected);

    mOutGateEdit = new CustomCompletionLineEdit(stationOutGateMatchModel, this);
    mOutGateEdit->setPlaceholderText(tr("Next segment"));
    connect(mOutGateEdit, &CustomCompletionLineEdit::indexSelected, this, &StopEditor::onOutGateSelected);

    mOutGateTrackSpin = new QSpinBox;
    mOutGateTrackSpin->setMaximum(0);
    connect(mOutGateTrackSpin, qOverload<int>(&QSpinBox::valueChanged), this, &StopEditor::startOutTrackTimer);
    connect(mOutGateTrackSpin, &QSpinBox::editingFinished, this, &StopEditor::checkOutGateTrack);

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
    lay->addWidget(mStTrackEdit, 1, 0, 1, 3);
    lay->addWidget(mOutGateEdit, 2, 0, 1, 2);
    lay->addWidget(mOutGateTrackSpin, 2, 2);

    setTabOrder(mStationEdit, arrEdit);
    setTabOrder(arrEdit, depEdit);
    setTabOrder(depEdit, mOutGateEdit);
}

StopEditor::~StopEditor()
{
    stopOutTrackTimer();
}

void StopEditor::setStop(const StopItem &item, const StopItem &prev)
{
    curStop = item;
    prevStop = prev;

    arrEdit->setToolTip(QString());
    switch (curStop.type)
    {
    case StopType::Normal:
    {
        arrEdit->setToolTip(tr("Press shift if you don't want to change also departure time."));
        arrEdit->setEnabled(true);
        depEdit->setEnabled(true);
        break;
    }
    case StopType::Transit:
    {
        arrEdit->setEnabled(true);

        depEdit->setEnabled(false);
        depEdit->setVisible(false);
        break;
    }
    case StopType::First:
    {
        arrEdit->setEnabled(false);
        arrEdit->setVisible(false);
        break;
    }
    case StopType::Last:
    {
        depEdit->setEnabled(false);
        depEdit->setVisible(false);

        mOutGateEdit->hide();
        if(curStop.stationId == 0)
            setFocusProxy(mStationEdit);
        break;
    }
    default:
        break;
    }

    if(curStop.type == StopType::First)
        stationsMatchModel->setFilter(0);
    else
        stationsMatchModel->setFilter(prevStop.stationId);
    mStationEdit->setData(curStop.stationId);

    stationTrackMatchModel->setFilter(curStop.stationId);
    mStTrackEdit->setData(curStop.trackId);
    mStTrackEdit->setEnabled(curStop.stationId != 0); //Enable only if station is selected

    stationOutGateMatchModel->setFilter(curStop.stationId, true, prevStop.nextSegment.segmentId, true);
    mOutGateEdit->setData(curStop.toGate.gateId);

    updateGateTrackSpin(curStop.toGate);

    //Set Arrival and Departure
    arrEdit->blockSignals(true);
    arrEdit->setTime(curStop.arrival);
    arrEdit->blockSignals(false);

    depEdit->setTime(curStop.departure);

    if(curStop.type != StopType::First)
    {
        /* Next stop must be at least one minute after
         * This is to prevent contemporary stops that will break ORDER BY arrival queries */
        const QTime minArr = prevStop.departure.addSecs(60);
        arrEdit->blockSignals(true);
        arrEdit->setMinimumTime(minArr);
        arrEdit->blockSignals(false);

        //First stop: arrival is hidden, you can change only departure so do not set a minimum
        //Normal stop: at least 1 minute stop
        //Transit, Last: departure = arrival
        QTime minDep = arrEdit->time();
        if(curStop.type == StopType::Normal)
            depEdit->setMinimumTime(minDep.addSecs(60));
        else
            depEdit->setMinimumTime(minDep);
    }
}

void StopEditor::updateStopArrDep()
{
    curStop.arrival = arrEdit->time();
    curStop.departure = depEdit->time();
}

void StopEditor::setCloseOnSegmentChosen(bool value)
{
    m_closeOnSegmentChosen = value;
}

void StopEditor::popupSegmentCombo()
{
    //This code is used when adding a new stop.
    //When user clicks on 'AddHere' a new stop is added
    //but before editing it, user must choose the railway segment
    //that the job will take from former Last Stop.
    //(It was Last Stop before we added this stop, so it didn't have a 'next segment')

    //1 - We popup lines combo from former last stop
    //2 - When user chooses a line we close the editor (emit lineChosen())
    //3 - We edit edit new Last Stop (EditNextItem)
    setCloseOnSegmentChosen(true);

    //Look for all possible segments
    stationOutGateMatchModel->autoSuggest(QString());

    const int count = stationOutGateMatchModel->rowCount();
    if(count > 1 && !stationOutGateMatchModel->isEmptyRow(0)
        && (stationOutGateMatchModel->isEmptyRow(1) || stationOutGateMatchModel->isEllipsesRow(1)))
    {
        //Only 1 segment available, use it
        db_id newSegId = stationOutGateMatchModel->getSegmentIdAtRow(0);

        db_id segOutGateId = 0;
        if(model->trySelectNextSegment(curStop, newSegId, 0, 0, segOutGateId))
        {
            //Success, close editor
            emit nextSegmentChosen(this);
            return;
        }
    }

    //We have multiple segments, let the user choose
    mOutGateEdit->showPopup();
}

void StopEditor::onStationSelected()
{
    db_id newStId = 0;
    QString tmp;
    if(!mStationEdit->getData(newStId, tmp))
        return;

    if(newStId == curStop.stationId)
        return;

    curStop.stationId = newStId;

    //Update track
    stationTrackMatchModel->setFilter(curStop.stationId);
    mStTrackEdit->setEnabled(curStop.stationId != 0); //Enable only if station is selected

    if(curStop.stationId)
    {
        if(!model->trySelectTrackForStop(curStop))
            curStop.trackId = 0; //Could not find a track

        mStTrackEdit->setData(curStop.trackId);
    }

    //Update prev segment
    prevStop.nextSegment = StopItem::Segment{}; //Reset, will be reloaded by model

    //Update next segment
    stationOutGateMatchModel->setFilter(curStop.stationId, true, prevStop.nextSegment.segmentId, true);
    mOutGateEdit->setData(0); //Reset, user must choose again

    curStop.nextSegment = StopItem::Segment{};
}

void StopEditor::onTrackSelected()
{
    db_id newTrackId = 0;
    QString tmp;
    if(!mStTrackEdit->getData(newTrackId, tmp))
        return;

    //Check if track is connected to gates
    if(!model->trySetTrackConnections(curStop, newTrackId, &tmp))
    {
        //Show error to the user
        bool stillSucceded = (curStop.trackId == newTrackId);
        QMessageBox::warning(this, stillSucceded ? tr("Gate Warning") : tr("Track Error"), tmp);

        if(!stillSucceded)
            mStTrackEdit->setData(curStop.trackId); //Reset to previous track
    }
}

void StopEditor::onOutGateSelected(const QModelIndex& idx)
{
    db_id newGateId = 0;
    QString gateSegmentName;
    if(!mOutGateEdit->getData(newGateId, gateSegmentName))
        return;

    const db_id newSegId = stationOutGateMatchModel->getSegmentIdAtRow(idx.row());
    const db_id oldGateId = curStop.toGate.gateId;
    db_id segOutGateId = 0;
    if(model->trySelectNextSegment(curStop, newSegId, 0, 0, segOutGateId))
    {
        //Update gate track
        updateGateTrackSpin(curStop.toGate);

        //Success, close editor
        emit nextSegmentChosen(this);
    }
    else
    {
        //Warn user and reset to previous chosen segment if any
        QMessageBox::warning(this, tr("Stop Error"), tr("Cannot set segment <b>%1</b>").arg(gateSegmentName));
        mOutGateEdit->setData(oldGateId);
    }
}

void StopEditor::checkOutGateTrack()
{
    stopOutTrackTimer();

    if(!curStop.nextSegment.segmentId)
        return; //First we need to have a segment

    int trackNum = mOutGateTrackSpin->value();
    curStop.toGate.trackNum = trackNum; //Trigger checking of railway segment connections

    db_id segOutGateId = 0;
    if(model->trySelectNextSegment(curStop, curStop.nextSegment.segmentId, trackNum, 0, segOutGateId))
    {
        //Update gate track
        updateGateTrackSpin(curStop.toGate);

        if(curStop.toGate.trackNum != trackNum)
        {
            //It wasn't possible to set requested track
            QMessageBox::warning(this, tr("Stop Error"),
                                 tr("Requested gate out track <b>%1</b> is not connected to segment <b>%2</b>.<br>"
                                    "Out track <b>%3</b> was chosen instead.<br>"
                                    "Look segment track connection from Stations Manager for more information"
                                    " on available tracks.")
                                 .arg(trackNum)
                                 .arg(mOutGateEdit->text())
                                 .arg(curStop.toGate.trackNum));
        }

        //Success, close editor
        emit nextSegmentChosen(this);
    }
    else
    {
        //Warn user and reset to previous chosen segment if any
        QMessageBox::warning(this, tr("Stop Error"), tr("Cannot set segment track!"));
    }
}

void StopEditor::arrivalChanged(const QTime& arrival)
{
    bool shiftPressed = QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
    QTime dep = depEdit->time();
    if(!shiftPressed)
    {
        //Shift departure by the same amount if SHIFT NOT pressed
        int diff = curStop.arrival.msecsTo(arrival);
        dep = dep.addMSecs(diff);
    }
    QTime minDep = arrival;
    if(curStop.type == StopType::Normal)
    {
        minDep = arrival.addSecs(60); //At least stop for 1 minute in Normal stops
    }
    depEdit->setMinimumTime(minDep);
    depEdit->setTime(dep); //Set after setting minimum time
    curStop.arrival = arrival; //Reset diff to 0 for next call
}

void StopEditor::startOutTrackTimer()
{
    //Give user a small time to scroll values in out gate track QSpinBox
    //This will skip eventual non available tracks (not connected)
    //On timeout check track and reset to old value if not available
    stopOutTrackTimer();
    mTimerOutTrack = startTimer(700);
}

void StopEditor::timerEvent(QTimerEvent *e)
{
    if(e->timerId() == mTimerOutTrack)
    {
        checkOutGateTrack();
        return;
    }

    QFrame::timerEvent(e);
}

void StopEditor::stopOutTrackTimer()
{
    if(mTimerOutTrack)
    {
        killTimer(mTimerOutTrack);
        mTimerOutTrack = 0;
    }
}

void StopEditor::updateGateTrackSpin(const StopItem::Gate &toGate)
{
    stopOutTrackTimer();

    int outTrackCount = 0;
    if(toGate.gateId)
        outTrackCount = stationOutGateMatchModel->getGateTrackCount(toGate.gateId);

    //Prevent trigger valueChanged() signal
    mOutGateTrackSpin->blockSignals(true);
    mOutGateTrackSpin->setMaximum(qMax(1, outTrackCount - 1)); //At least one track
    mOutGateTrackSpin->setValue(toGate.trackNum);
    mOutGateTrackSpin->blockSignals(false);
}
