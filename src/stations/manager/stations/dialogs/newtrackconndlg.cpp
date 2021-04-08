#include "newtrackconndlg.h"

#include <QMessageBox>
#include <QDialogButtonBox>

#include <QGridLayout>
#include <QFormLayout>

#include <QGroupBox>

#include <QComboBox>
#include <QSpinBox>

#include "utils/sqldelegate/customcompletionlineedit.h"
#include "utils/sqldelegate/isqlfkmatchmodel.h"

#include "stations/match_models/stationgatesmatchmodel.h"

#include "stations/station_name_utils.h"


NewTrackConnDlg::NewTrackConnDlg(ISqlFKMatchModel *tracks,
                                 StationGatesMatchModel *gates,
                                 QWidget *parent) :
    QDialog(parent),
    trackMatchModel(tracks),
    gatesMatchModel(gates)
{
    QStringList sideTypeEnum;
    sideTypeEnum.reserve(int(utils::Side::NSides));
    for(int i = 0; i < int(utils::Side::NSides); i++)
        sideTypeEnum.append(utils::StationUtils::name(utils::Side(i)));

    trackMatchModel->setHasEmptyRow(false);
    gatesMatchModel->setHasEmptyRow(false);

    QGridLayout *mainLay = new QGridLayout(this);

    QGroupBox *trackBox = new QGroupBox(tr("Track"));
    QFormLayout *trackLay = new QFormLayout(trackBox);

    trackEdit = new CustomCompletionLineEdit(trackMatchModel);
    trackEdit->setPlaceholderText(tr("Track..."));
    trackLay->addRow(trackEdit);

    trackSideCombo = new QComboBox;
    trackSideCombo->addItems(sideTypeEnum);
    trackSideCombo->setCurrentIndex(0);
    trackLay->addRow(tr("Track Side:"), trackSideCombo);

    QGroupBox *gateBox = new QGroupBox(tr("Gate"));
    QFormLayout *gateLay = new QFormLayout(gateBox);

    gateEdit = new CustomCompletionLineEdit(gatesMatchModel);
    gateEdit->setPlaceholderText(tr("Gate..."));
    connect(gateEdit, &CustomCompletionLineEdit::dataIdChanged, this, &NewTrackConnDlg::onGateChanged);
    gateLay->addRow(gateEdit);

    gateTrackSpin = new QSpinBox;
    gateTrackSpin->setRange(0, 999);
    gateLay->addRow(tr("Gate Track:"), gateTrackSpin);

    mainLay->addWidget(trackBox, 0, 0);
    mainLay->addWidget(gateBox, 0, 1);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                 Qt::Horizontal);
    connect(box, &QDialogButtonBox::accepted, this, &NewTrackConnDlg::accept);
    connect(box, &QDialogButtonBox::rejected, this, &NewTrackConnDlg::reject);
    mainLay->addWidget(box, 1, 0, 1, 2);

    setMinimumSize(200, 100);
    setWindowTitle(tr("New Station Track Connection"));
}

void NewTrackConnDlg::done(int res)
{
    if(res == QDialog::Accepted)
    {
        //Prevent closing if data is not valid
        db_id tmpId;
        QString tmpName;

        if(!trackEdit->getData(tmpId, tmpName))
        {
            QMessageBox::warning(this, tr("Track Connection Error"),
                                 tr("Please set a valid track."));
            return;
        }
        if(!gateEdit->getData(tmpId, tmpName))
        {
            QMessageBox::warning(this, tr("Track Connection Error"),
                                 tr("Please set a valid gate."));
            return;
        }
    }

    QDialog::done(res);
}

void NewTrackConnDlg::getData(db_id &trackOut, utils::Side &trackSideOut, db_id &gateOut, int &gateTrackOut)
{
    QString tmp;
    trackEdit->getData(trackOut, tmp);
    trackSideOut = utils::Side(trackSideCombo->currentIndex());
    gateEdit->getData(gateOut, tmp);
    gateTrackOut = gateTrackSpin->value();
}

void NewTrackConnDlg::onGateChanged(db_id gateId)
{
    //NOTE HACK:
    //CustomCompletionLineEdit doesn't allow getting custom data
    //Ask model directly before it's cleared by CustomCompletionLineEdit
    int outTrackCount = gatesMatchModel->getOutTrackCount(gateId);
    if(outTrackCount == 0)
        outTrackCount = 1000; //Set to some value
    gateTrackSpin->setMaximum(outTrackCount - 1);
}
