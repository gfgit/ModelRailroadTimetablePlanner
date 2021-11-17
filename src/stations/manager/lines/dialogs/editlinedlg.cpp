#include "editlinedlg.h"

#include <QBoxLayout>
#include <QFormLayout>

#include <QLineEdit>
#include "utils/spinbox/kmspinbox.h"
#include <QTableView>
#include <QToolButton>

#include <QTabWidget>
#include <QDialogButtonBox>

#include <QMessageBox>

#include "stations/manager/lines/model/linesegmentsmodel.h"
#include "stations/manager/lines/dialogs/choosesegmentdlg.h"

#include <QPointer>


EditLineDlg::EditLineDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    mDb(db)
{
    model = new LineSegmentsModel(mDb, this);

    //Details Tab
    QWidget *detailsTab = new QWidget;
    QFormLayout *detailsTabLay = new QFormLayout(detailsTab);

    lineNameEdit = new QLineEdit;
    lineNameEdit->setPlaceholderText(tr("Name..."));
    detailsTabLay->addRow(tr("Name:"), lineNameEdit);

    lineStartKmSpin = new KmSpinBox;
    lineStartKmSpin->setPrefix(tr("Km "));
    detailsTabLay->addRow(tr("Start at:"), lineStartKmSpin);

    //Path Tab
    QWidget *pathTab = new QWidget;
    QVBoxLayout *pathTabLay = new QVBoxLayout(pathTab);

    //Buttons
    QHBoxLayout *toolsLay = new QHBoxLayout;

    QToolButton *addStationBut = new QToolButton;
    addStationBut->setText(tr("Add station"));
    toolsLay->addWidget(addStationBut);
    QToolButton *removeFromPosBut = new QToolButton;
    removeFromPosBut->setText(tr("Remove After"));
    toolsLay->addWidget(removeFromPosBut);
    toolsLay->addStretch();

    pathTabLay->addLayout(toolsLay);

    view = new QTableView;
    view->setModel(model);

    pathTabLay->addWidget(view);

    QVBoxLayout *lay = new QVBoxLayout(this);
    QTabWidget *tabWidget = new QTabWidget;
    tabWidget->addTab(detailsTab, tr("Details"));
    tabWidget->addTab(pathTab, tr("Path"));
    lay->addWidget(tabWidget);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal);
    lay->addWidget(box);

    connect(model, &LineSegmentsModel::modelError, this, &EditLineDlg::onModelError);

    connect(box, &QDialogButtonBox::accepted, this, &EditLineDlg::accept);
    connect(box, &QDialogButtonBox::rejected, this, &EditLineDlg::reject);

    connect(addStationBut, &QToolButton::clicked, this, &EditLineDlg::addStation);
    connect(removeFromPosBut, &QToolButton::clicked, this, &EditLineDlg::removeAfterCurrentPos);

    setWindowTitle(tr("Edit Line"));
    setMinimumSize(500, 300);
}

void EditLineDlg::done(int res)
{
    //FIXME: cannot cancel editings made to path so always accept
    res = QDialog::Accepted;

    if(res == QDialog::Accepted)
    {
        QString lineName = lineNameEdit->text().simplified();
        int startMeters = lineStartKmSpin->value();

        if(lineName.isEmpty())
        {
            onModelError(tr("Line name cannot be empty."));
            return;
        }

        if(!model->setLineInfo(lineName, startMeters))
        {
            return; //Error
        }
    }

    QDialog::done(res);
}

void EditLineDlg::setLineId(db_id lineId)
{
    model->setLine(lineId);

    //Reload UI
    QString lineName;
    int startMeters = 0;
    if(!model->getLineInfo(lineName, startMeters))
    {
        return; //Error
    }

    lineNameEdit->setText(lineName);
    lineStartKmSpin->setValue(startMeters);
}

void EditLineDlg::onModelError(const QString &msg)
{
    QMessageBox::warning(this, tr("Error"), msg);
}

void EditLineDlg::addStation()
{
    if(model->getSegmentCount() >= MaxSegmentsPerLine)
    {
        QMessageBox::warning(this, tr("Error"),
                             tr("Cannot add another segment to line <b>%1</b>.<br>"
                                "Each line can have a maximum of <b>%2 segments</b>.")
                             .arg(lineNameEdit->text())
                                 .arg(MaxSegmentsPerLine));
        return;
    }

    db_id lastStationId = model->getLastStation();
    db_id lastSegmentId = model->getLastRailwaySegment();

    QPointer<ChooseSegmentDlg> dlg(new ChooseSegmentDlg(mDb, this));
    dlg->setFilter(lastStationId, ChooseSegmentDlg::DoNotLock, lastSegmentId);
    int ret = dlg->exec();
    if(!dlg)
        return;

    if(ret != QDialog::Accepted)
    {
        delete dlg;
        return;
    }

    db_id segmentId = 0;
    QString segmentName;
    bool isReversed = false;

    if(dlg->getData(segmentId, segmentName, isReversed))
    {
        model->addStation(segmentId, isReversed);
    }

    delete dlg;
}

void EditLineDlg::removeAfterCurrentPos()
{
    if(!view->selectionModel()->hasSelection())
    {
        onModelError(tr("Please select a segment to remove.\n"
                        "All segments after it are also removed."));
        return;
    }

    QModelIndex idx = view->currentIndex();
    if(!idx.isValid())
        return;

    int pos = model->getItemIndex(idx.row());
    const int segmentCount = model->getSegmentCount();
    if(pos >= segmentCount)
        pos = segmentCount - 1; //Remove only the last one
    const QString stationName = model->getStationNameAt(pos);

    const QString msg =
        tr("Remove last <b>%1</b> segments?"
           "%2")
            .arg(segmentCount - pos)
            .arg(stationName.isEmpty()
                     ? QString()
                     : tr("<br>(From station <b>%1</b>)").arg(stationName));

    int ret = QMessageBox::question(this, tr("Are you sure?"), msg);
    if(ret != QMessageBox::Yes)
        return;

    model->removeSegmentsAfterPosInclusive(pos);
}
