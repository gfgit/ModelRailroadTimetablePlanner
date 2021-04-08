#include "choosesegmentdlg.h"

#include <QFormLayout>
#include "utils/sqldelegate/customcompletionlineedit.h"
#include <QDialogButtonBox>

#include "stations/stationsmatchmodel.h"
#include "stations/railwaysegmentmatchmodel.h"

#include <QMessageBox>

ChooseSegmentDlg::ChooseSegmentDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    lockFromStationId(0),
    lockToStationId(0),
    excludeSegmentId(0),
    isReversed(false)
{
    QFormLayout *lay = new QFormLayout(this);

    fromStationMatch = new StationsMatchModel(db, this);
    fromStationEdit = new CustomCompletionLineEdit(fromStationMatch);
    fromStationEdit->setPlaceholderText(tr("Filter..."));
    lay->addRow(tr("From station:"), fromStationEdit);

    toStationMatch = new StationsMatchModel(db, this);
    toStationEdit = new CustomCompletionLineEdit(fromStationMatch);
    toStationEdit->setPlaceholderText(tr("Filter..."));
    lay->addRow(tr("To station:"), toStationEdit);

    segmentMatch = new RailwaySegmentMatchModel(db, this);
    segmentEdit = new CustomCompletionLineEdit(segmentMatch);
    segmentEdit->setPlaceholderText(tr("Select..."));
    lay->addRow(tr("Segment:"), segmentEdit);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal);
    lay->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, &ChooseSegmentDlg::accept);
    connect(box, &QDialogButtonBox::rejected, this, &ChooseSegmentDlg::reject);

    connect(fromStationEdit, &CustomCompletionLineEdit::dataIdChanged, this, &ChooseSegmentDlg::onStationChanged);
    connect(toStationEdit, &CustomCompletionLineEdit::dataIdChanged, this, &ChooseSegmentDlg::onStationChanged);
    connect(segmentEdit, &CustomCompletionLineEdit::dataIdChanged, this, &ChooseSegmentDlg::onSegmentSelected);

    setWindowTitle(tr("Choose Railway Segment"));
    setMinimumSize(300, 100);
}

void ChooseSegmentDlg::done(int res)
{
    if(res == QDialog::Accepted)
    {
        db_id segmentId = 0;
        QString tmp;
        if(!segmentEdit->getData(segmentId, tmp))
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Invalid railway segment."));
            return;
        }
    }

    QDialog::done(res);
}

void ChooseSegmentDlg::setFilter(db_id fromStationId, db_id toStationId, db_id exceptSegment)
{
    lockFromStationId = fromStationId;
    lockToStationId = toStationId;
    if(!lockFromStationId)
        lockToStationId = DoNotLock; //NOTE: do not lock only destination
    excludeSegmentId = exceptSegment;

    fromStationEdit->setData(lockFromStationId);
    fromStationEdit->setReadOnly(lockFromStationId != DoNotLock);

    toStationEdit->setData(lockToStationId);
    toStationEdit->setReadOnly(lockToStationId != DoNotLock);

    fromStationMatch->setFilter(0);
    toStationMatch->setFilter(0);
    segmentMatch->setFilter(lockFromStationId, lockToStationId, excludeSegmentId);
}

bool ChooseSegmentDlg::getData(db_id &outSegId, QString &segName, bool &outIsReversed)
{
    segmentEdit->getData(outSegId, segName);
    outIsReversed = isReversed;
    return outSegId != 0;
}

void ChooseSegmentDlg::onStationChanged()
{
    QString tmp;
    db_id fromStationId = 0;
    db_id toStationId = 0;

    fromStationEdit->getData(fromStationId, tmp);
    toStationEdit->getData(toStationId, tmp);

    if(!fromStationId && !toStationEdit->isReadOnly())
    {
        //Do not filter only by destination
        toStationEdit->setReadOnly(true);
        toStationEdit->setPlaceholderText(tr("Set From station"));

        if(toStationId != 0)
        {
            //This will recurse so return
            toStationEdit->setData(0);
            return;
        }
    }
    else if(fromStationId && toStationEdit->isReadOnly())
    {
        //Re enable
        toStationEdit->setReadOnly(lockToStationId != DoNotLock);
        toStationEdit->setPlaceholderText(tr("Filter..."));
    }

    //Reset segment
    segmentEdit->setData(0);
    segmentMatch->setFilter(fromStationId, toStationId, excludeSegmentId);
    isReversed = false;
}

void ChooseSegmentDlg::onSegmentSelected(db_id segmentId)
{
    //NOTE HACK:
    //CustomCompletionLineEdit doesn't allow getting custom data
    //Ask model directly before it's cleared by CustomCompletionLineEdit
    isReversed = false;
    if(segmentId)
        isReversed = segmentMatch->isReversed(segmentId);
}
