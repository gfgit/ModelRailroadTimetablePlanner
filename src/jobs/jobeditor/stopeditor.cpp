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
    model(m),
    m_closeOnSegmentChosen(false)
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
    curStop = item;
    prevStop = prev;

    arrEdit->setToolTip(QString());
    switch (item.type)
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

        mSegmentEdit->hide();
        if(item.stationId == 0)
            setFocusProxy(mStationEdit);
        break;
    }
    default:
        break;
    }

    if(item.type == StopType::First)
        stationsMatchModel->setFilter(0);
    else
        stationsMatchModel->setFilter(prevStop.stationId);
    mStationEdit->setData(item.stationId);

    stationTrackMatchModel->setFilter(item.stationId);
    mTrackEdit->setData(item.trackId);
    mTrackEdit->setEnabled(item.stationId != 0); //Enable only if station is selected

    segmentMatchModel->setFilter(item.stationId, 0, 0);
    mSegmentEdit->setData(item.nextSegment.segmentId);

    //Set Arrival and Departure
    arrEdit->blockSignals(true);
    arrEdit->setTime(item.arrival);
    arrEdit->blockSignals(false);

    depEdit->setTime(item.departure);

    if(item.type != StopType::First)
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
        if(item.type == StopType::Normal)
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
    segmentMatchModel->autoSuggest(QString());

    const int count = segmentMatchModel->rowCount();
    if(count > 1 && !segmentMatchModel->isEmptyRow(0)
        && (segmentMatchModel->isEmptyRow(1) || segmentMatchModel->isEllipsesRow(1)))
    {
        //Only 1 segment available, use it
        db_id newSegId = segmentMatchModel->getIdAtRow(0);

        db_id outGateId = 0;
        if(model->trySelectNextSegment(curStop, newSegId, 0, outGateId))
        {
            //Success, close editor
            emit nextSegmentChosen(this);
        }
    }

    //We have multiple segments, let the user choose
    mSegmentEdit->showPopup();
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
    mTrackEdit->setEnabled(curStop.stationId != 0); //Enable only if station is selected

    if(curStop.stationId)
    {
        if(!model->trySelectTrackForStop(curStop))
            curStop.trackId = 0; //Could not find a track

        mTrackEdit->setData(curStop.trackId);
    }

    //Update prev segment
    prevStop.nextSegment = StopItem::Segment{}; //Reset, will be reloaded by model

    //Update next segment
    segmentMatchModel->setFilter(curStop.stationId, 0, 0);
    mSegmentEdit->setData(0); //Reset, user must choose again

    curStop.nextSegment = StopItem::Segment{};
}

void StopEditor::onTrackSelected()
{
    db_id newTrackId = 0;
    QString tmp;
    if(!mTrackEdit->getData(newTrackId, tmp))
        return;

    //Check if track is connected to gates
    if(!model->trySetTrackConnections(curStop, newTrackId, &tmp))
    {
        //Show error to the user
        bool stillSucceded = (curStop.trackId == newTrackId);
        QMessageBox::warning(this, stillSucceded ? tr("Gate Warning") : tr("Track Error"), tmp);

        if(!stillSucceded)
            mTrackEdit->setData(curStop.trackId); //Reset to previous track
    }
}

void StopEditor::onNextSegmentSelected()
{
    db_id newSegId = 0;
    QString segmentName;
    if(!mSegmentEdit->getData(newSegId, segmentName))
        return;

    const db_id oldSegId = curStop.nextSegment.segmentId;
    db_id outGateId = 0;
    if(model->trySelectNextSegment(curStop, newSegId, 0, outGateId))
    {
        emit nextSegmentChosen(this);
    }
    else
    {
        //Warn user and reset to previous chosen segment if any
        QMessageBox::warning(this, tr("Stop Error"), tr("Cannot set segment '%1'").arg(segmentName));
        mSegmentEdit->setData(oldSegId);
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
}
