#include "choosesegmentdlg.h"

#include <QFormLayout>
#include "utils/sqldelegate/customcompletionlineedit.h"
#include <QDialogButtonBox>

#include "stations/match_models/stationsmatchmodel.h"
#include "stations/match_models/stationgatesmatchmodel.h"

#include <QMessageBox>

ChooseSegmentDlg::ChooseSegmentDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    lockFromStationId(0),
    excludeSegmentId(0),
    selectedSegmentId(0),
    isReversed(false)
{
    QFormLayout *lay = new QFormLayout(this);

    fromStationMatch = new StationsMatchModel(db, this);
    fromStationEdit = new CustomCompletionLineEdit(fromStationMatch);
    fromStationEdit->setPlaceholderText(tr("Filter..."));
    lay->addRow(tr("From station:"), fromStationEdit);

    gateMatch = new StationGatesMatchModel(db, this);
    outGateEdit = new CustomCompletionLineEdit(gateMatch);
    outGateEdit->setPlaceholderText(tr("Select..."));
    lay->addRow(tr("Segment:"), outGateEdit);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal);
    lay->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, &ChooseSegmentDlg::accept);
    connect(box, &QDialogButtonBox::rejected, this, &ChooseSegmentDlg::reject);

    connect(fromStationEdit, &CustomCompletionLineEdit::dataIdChanged, this, &ChooseSegmentDlg::onStationChanged);
    connect(outGateEdit, &CustomCompletionLineEdit::indexSelected, this, &ChooseSegmentDlg::onSegmentSelected);

    setWindowTitle(tr("Choose Railway Segment"));
    setMinimumSize(300, 100);
}

void ChooseSegmentDlg::done(int res)
{
    if(res == QDialog::Accepted)
    {
        db_id segmentId = 0;
        QString tmp;
        if(!outGateEdit->getData(segmentId, tmp))
        {
            QMessageBox::warning(this, tr("Error"),
                                 tr("Invalid railway segment."));
            return;
        }
    }

    QDialog::done(res);
}

void ChooseSegmentDlg::setFilter(db_id fromStationId, db_id exceptSegment)
{
    lockFromStationId = fromStationId;
    excludeSegmentId = exceptSegment;
    selectedSegmentId = 0;

    fromStationEdit->setData(lockFromStationId);
    fromStationEdit->setReadOnly(lockFromStationId != DoNotLock);

    fromStationMatch->setFilter(0);
    gateMatch->setFilter(lockFromStationId, true, excludeSegmentId, true);
}

bool ChooseSegmentDlg::getData(db_id &outSegId, QString &segName, bool &outIsReversed)
{
    db_id tmpGateId = 0;
    outGateEdit->getData(tmpGateId, segName);
    outSegId = selectedSegmentId;
    outIsReversed = isReversed;
    return outSegId != 0;
}

void ChooseSegmentDlg::onStationChanged()
{
    QString tmp;
    fromStationEdit->getData(lockFromStationId, tmp);

    //Reset segment
    outGateEdit->setData(0);
    gateMatch->setFilter(lockFromStationId, true, excludeSegmentId, true);
    isReversed = false;
}

void ChooseSegmentDlg::onSegmentSelected(const QModelIndex& idx)
{
    isReversed = false;
    selectedSegmentId = gateMatch->getSegmentIdAtRow(idx.row());
    if(selectedSegmentId)
        isReversed = gateMatch->isSegmentReversedAtRow(idx.row());
}
