#include "stopeditor.h"

#include "stopeditinghelper.h"

#include "utils/delegates/sql/customcompletionlineedit.h"

#include <QTimeEdit>
#include <QSpinBox>
#include <QGridLayout>


StopEditor::StopEditor(sqlite3pp::database &db, StopModel *m, QWidget *parent) :
    QFrame(parent),
    m_closeOnSegmentChosen(false)
{
    mOutGateTrackSpin = new QSpinBox;
    arrEdit = new QTimeEdit;
    depEdit = new QTimeEdit;

    helper = new StopEditingHelper(db, m,
                                   mOutGateTrackSpin, arrEdit, depEdit,
                                   this);
    connect(helper, &StopEditingHelper::nextSegmentChosen, this, &StopEditor::onHelperSegmentChoosen);

#ifdef PRINT_DBG_MSG
    setObjectName(QStringLiteral("StopEditor (%1)").arg(qintptr(this)));
#endif

    setFrameShape(QFrame::Box);

    CustomCompletionLineEdit *mStationEdit = helper->getStationEdit();
    CustomCompletionLineEdit *mStTrackEdit = helper->getStTrackEdit();
    CustomCompletionLineEdit *mOutGateEdit = helper->getOutGateEdit();

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

    setToolTip(tr("To avoid recalculation of travel times when saving changes, hold SHIFT modifier while closing editor"));
}

StopEditor::~StopEditor()
{
    delete helper;
    helper = nullptr;
}

void StopEditor::setStop(const StopItem &item, const StopItem &prev)
{
    helper->setStop(item, prev);

    if(item.stationId == 0)
        setFocusProxy(helper->getStationEdit());
}

const StopItem &StopEditor::getCurItem() const
{
    return helper->getCurItem();
}

const StopItem &StopEditor::getPrevItem() const
{
    return helper->getPrevItem();
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
    helper->popupSegmentCombo();
}

void StopEditor::onHelperSegmentChoosen()
{
    //Forward signal and pass self instance
    emit nextSegmentChosen(this);
}
