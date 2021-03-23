#include "stopeditor.h"

#include "utils/sqldelegate/customcompletionlineedit.h"

#include "stations/stationsmatchmodel.h"
#include "model/stationlineslistmodel.h"

#include <QTimeEdit>
#include <QComboBox>
#include <QAbstractItemView>
#include <QGridLayout>
#include <QMenu>

#include <QMouseEvent>

#include <QGuiApplication>

#include "app/scopedebug.h"

StopEditor::StopEditor(sqlite3pp::database &db, QWidget *parent) :
    QFrame(parent),
    linesModel(nullptr),
    curLineId(0),
    nextLineId(0),
    stopId(0),
    stationId(0),
    mPrevSt(0),
    stopType(Normal),
    m_closeOnLineChosen(false)
{
    stationsMatchModel = new StationsMatchModel(db, this);
    mLineEdit = new CustomCompletionLineEdit(stationsMatchModel, this);
    mLineEdit->setPlaceholderText(tr("Station name"));

    connect(mLineEdit, &CustomCompletionLineEdit::dataIdChanged, this, &StopEditor::onStationSelected);

    arrEdit = new QTimeEdit;
    depEdit = new QTimeEdit;
    connect(arrEdit, &QTimeEdit::timeChanged, this, &StopEditor::arrivalChanged);
#ifdef PRINT_DBG_MSG
    setObjectName(QStringLiteral("StopEditor (%1)").arg(qintptr(this)));
    arrEdit->setObjectName(QStringLiteral("QTimeEdit (%1)").arg(qintptr(arrEdit)));
    depEdit->setObjectName(QStringLiteral("QTimeEdit (%1)").arg(qintptr(depEdit)));
#endif
    linesCombo = new QComboBox;
    linesModel = new StationLinesListModel(db, this);
    linesCombo->setModel(linesModel);
    connect(linesModel, &StationLinesListModel::resultsReady, this, &StopEditor::linesLoaded);

    connect(linesCombo, QOverload<int>::of(&QComboBox::activated), this, &StopEditor::onNextLineChanged);
    QAbstractItemView *view = linesCombo->view();
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    view->viewport()->installEventFilter(this);
    connect(view, &QAbstractItemView::customContextMenuRequested, this, &StopEditor::comboBoxViewContextMenu);

    setFrameShape(QFrame::Box);

    lay = new QGridLayout(this);
    lay->addWidget(mLineEdit, 0, 0);
    lay->addWidget(arrEdit, 0, 1);
    lay->addWidget(depEdit, 0, 2);
    lay->addWidget(linesCombo, 1, 0, 1, 3);

    setTabOrder(mLineEdit, arrEdit);
    setTabOrder(arrEdit, depEdit);
    setTabOrder(depEdit, linesCombo);
}

bool StopEditor::eventFilter(QObject *watched, QEvent *ev)
{
    //Filter out right click to prevent cobobox popup from closoing when opening context menu
    if(ev->type() == QEvent::MouseButtonRelease && watched == linesCombo->view()->viewport())
    {
        if(static_cast<QMouseEvent*>(ev)->button() == Qt::RightButton)
            return true;
    }
    return false;
}

void StopEditor::setPrevDeparture(const QTime &prevTime)
{
    /* Next stop must be at least one minute after
     * This is to prevent contemporary stops that will break ORDER BY arrival queries */
    const QTime minTime = prevTime.addSecs(60);
    arrEdit->blockSignals(true);
    arrEdit->setMinimumTime(minTime);
    arrEdit->blockSignals(false);

    //If not Transit, at least 1 minute stop, First and Last have the same time for Arrival and Departure
    if(stopType == Normal)
        depEdit->setMinimumTime(minTime.addSecs(60));
    else
        depEdit->setMinimumTime(minTime);
}

void StopEditor::setPrevSt(db_id stId)
{
    mPrevSt = stId;
}

void StopEditor::setCurLine(db_id lineId)
{
    curLineId = lineId;
    if(nextLineId == 0)
        nextLineId = lineId;
}

void StopEditor::setNextLine(db_id lineId)
{
    nextLineId = lineId;
    if(curLineId == 0)
        setCurLine(lineId);
}

void StopEditor::setStopType(int type)
{
    stopType = type;
}

void StopEditor::calcInfo()
{
    arrEdit->setToolTip(QString());
    switch (stopType)
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

        lay->removeWidget(linesCombo);
        linesCombo->hide();
        if(stationId == 0)
            setFocusProxy(mLineEdit);
        break;
    }
    default:
        break;
    }

    mLineEdit->setText(stationsMatchModel->getName(stationId));
    linesModel->setStationId(stationId);

    int row = -1;
    if(nextLineId != 0)
    {
        row = linesModel->getLineRow(nextLineId);
    }
    if(row < 0)
    {
        row = linesModel->getLineRow(curLineId);
    }

    if(stopType == First)
        stationsMatchModel->setFilter(0);
    else
        stationsMatchModel->setFilter(mPrevSt);

    if(row < 0)
        row = 0;
    linesCombo->setCurrentIndex(row);

    if(stopType != First)
    {
        //First stop: arrival is hidden, you can change only departure so do not set a minimum
        //Normal stop: at least 1 minute stop
        //Transit, Last: departure = arrival
        QTime minDep = arrEdit->time();
        if(stopType == Normal)
            depEdit->setMinimumTime(minDep.addSecs(60));
        else
            depEdit->setMinimumTime(minDep);
    }
}

QTime StopEditor::getArrival()
{
    return arrEdit->time();
}

QTime StopEditor::getDeparture()
{
    return depEdit->time();
}

db_id StopEditor::getCurLine()
{
    return curLineId;
}

db_id StopEditor::getNextLine()
{
    qDebug() << "Get NextLine:" << nextLineId;
    return nextLineId;
}

db_id StopEditor::getStation()
{
    return stationId;
}

void StopEditor::popupLinesCombo()
{
    //This code is used when adding a new stop.
    //When user clicks on 'AddHere' a new stop is added
    //but before editing it, user must choose the line
    //that the job will take from former Last Stop.
    //(It was Lst Stop before we added this stop,
    //so it didn't have a 'next line')

    //1 - We popup lines combo from former last stop
    //2 - When user chooses a line we close the editor (emit lineChosen())
    //3 - We edit edit new Last Stop (EditNextItem)
    setCloseOnLineChosen(true);

    if(linesCombo->count() == 1)
    {
        //If there is only one possible 'next line'
        //then it is the same as 'current line' so
        //it is already set, just tell the delegate
        emit lineChosen(this);
    }
    else
    {
        //Show available options
        linesCombo->showPopup();
    }
}

void StopEditor::setArrival(const QTime &arr)
{
    arrEdit->blockSignals(true);
    arrEdit->setTime(arr);
    lastArrival = arr;
    arrEdit->blockSignals(false);
}

void StopEditor::setDeparture(const QTime &dep)
{
    depEdit->setTime(dep);
}

void StopEditor::setStation(db_id stId)
{
    stationId = stId;
}

void StopEditor::onStationSelected(db_id stId)
{
    DEBUG_ENTRY;
    if(stId <= 0 || stId == stationId)
        return;

    stationId = stId;
    linesModel->setStationId(stationId);

    /*Try to set ComboBox to old value if it is still present
     * Example:
     * St: Venezia    Line: VE-PD_LL
     *
     * User changes St to Mestre
     * Try to set Combo to 'VE-PD_LL' if new St has this line
     * Fallback to CurLine or NextLine or nothing (First index)
    */
    int row = linesModel->getLineRow(nextLineId);
    if(row < 0)
    {
        row = linesModel->getLineRow(curLineId);
        nextLineId = curLineId;
    }
    if(row < 0)
    {
        nextLineId = 0;
    }

    if(!curLineId)
        row = 0;

    linesCombo->setCurrentIndex(row);
}

bool StopEditor::closeOnLineChosen() const
{
    return m_closeOnLineChosen;
}

void StopEditor::setCloseOnLineChosen(bool value)
{
    m_closeOnLineChosen = value;
}

void StopEditor::onNextLineChanged(int index)
{
    DEBUG_ENTRY;
    nextLineId = linesModel->getLineIdAt(index);
    qDebug() << "NextLine:" << nextLineId << "(" << index << ")";
    emit lineChosen(this);
}

void StopEditor::comboBoxViewContextMenu(const QPoint& pos)
{
    int curPage = linesModel->currentPage();
    int pageCount = linesModel->getPageCount();
    if(pageCount < 1)
        pageCount = 1;

    QMenu menu(this);
    QAction *prevPage = menu.addAction(tr("Previous page"));
    QAction *nextPage = menu.addAction(tr("Next page"));
    QAction *pageInfo = menu.addAction(tr("Page: %1/%2").arg(curPage + 1).arg(pageCount));

    prevPage->setEnabled(curPage > 0);
    nextPage->setEnabled(curPage < pageCount - 1);
    pageInfo->setEnabled(false); //Just info as a label

    QAction *act = menu.exec(linesCombo->view()->viewport()->mapToGlobal(pos));
    if(act == prevPage)
    {
        linesModel->switchToPage(curPage - 1);
        nextLineId = 0;
    }
    else if(act == nextPage)
    {
        linesModel->switchToPage(curPage + 1);
        nextLineId = 0;
    }
}

void StopEditor::linesLoaded()
{
    if(nextLineId)
        return;
    nextLineId = linesModel->getLineIdAt(linesCombo->currentIndex());
}

void StopEditor::arrivalChanged(const QTime& arrival)
{
    bool shiftPressed = QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
    QTime dep = depEdit->time();
    if(!shiftPressed)
    {
        //Shift departure by the same amount if SHIFT NOT pressed
        int diff = lastArrival.msecsTo(arrival);
        dep = dep.addMSecs(diff);
    }
    QTime minDep = arrival;
    if(stopType == Normal)
    {
        minDep = arrival.addSecs(60); //At least stop for 1 minute in Normal stops
    }
    depEdit->setMinimumTime(minDep);
    depEdit->setTime(dep); //Set after setting minimum time
    lastArrival = arrival;
}
