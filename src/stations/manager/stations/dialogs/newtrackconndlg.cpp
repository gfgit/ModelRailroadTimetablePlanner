/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "newtrackconndlg.h"

#include <QMessageBox>
#include <QDialogButtonBox>

#include <QGridLayout>
#include <QFormLayout>

#include <QGroupBox>

#include <QComboBox>
#include <QSpinBox>

#include "utils/delegates/sql/customcompletionlineedit.h"
#include "utils/delegates/sql/isqlfkmatchmodel.h"

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
    trackLay->addRow(trackEdit);

    trackSideCombo = new QComboBox;
    trackSideCombo->addItems(sideTypeEnum);
    trackSideCombo->setCurrentIndex(0);
    connect(trackSideCombo, QOverload<int>::of(&QComboBox::activated),
            this, &NewTrackConnDlg::onTrackSideChanged);
    trackLay->addRow(tr("Track Side:"), trackSideCombo);

    QGroupBox *gateBox = new QGroupBox(tr("Gate"));
    QFormLayout *gateLay = new QFormLayout(gateBox);

    gateEdit = new CustomCompletionLineEdit(gatesMatchModel);
    connect(gateEdit, &CustomCompletionLineEdit::dataIdChanged,
            this, &NewTrackConnDlg::onGateChanged);
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

    setMode(SingleConnection);

    setMinimumSize(200, 100);
}

void NewTrackConnDlg::done(int res)
{
    if(res == QDialog::Accepted)
    {
        //Prevent closing if data is not valid
        db_id tmpId;
        QString tmpName;

        if(m_mode != GateToAllTracks && !trackEdit->getData(tmpId, tmpName))
        {
            QMessageBox::warning(this, tr("Track Connection Error"),
                                 tr("Please set a valid track."));
            return;
        }
        if(m_mode != TrackToAllGates && !gateEdit->getData(tmpId, tmpName))
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

void NewTrackConnDlg::setMode(NewTrackConnDlg::Mode mode)
{
    m_mode = mode;

    if(m_mode == GateToAllTracks)
    {
        trackEdit->setPlaceholderText(tr("All Tracks"));
        trackEdit->setData(0);
        trackEdit->setReadOnly(true);
        //Gate decides which side
        trackSideCombo->setEnabled(false);
    }
    else
    {
        trackEdit->setPlaceholderText(tr("Track..."));
        trackEdit->setReadOnly(false);
        trackSideCombo->setEnabled(true);
    }

    if(m_mode == TrackToAllGates)
    {
        gateEdit->setData(0);
        gateEdit->setReadOnly(true);
        //Force update placeholder text
        onTrackSideChanged();
    }
    else
    {
        gateEdit->setPlaceholderText(tr("Gate..."));
        gateEdit->setReadOnly(false);
    }

    switch (m_mode)
    {
    case SingleConnection:
        setWindowTitle(tr("New Station Track Connection"));
        break;
    case TrackToAllGates:
        setWindowTitle(tr("Station Track To All Gates"));
        break;
    case GateToAllTracks:
        setWindowTitle(tr("Gate To All Station Tracks"));
        break;
    }
}

void NewTrackConnDlg::onGateChanged(db_id gateId)
{
    //NOTE HACK:
    //CustomCompletionLineEdit doesn't allow getting custom data
    //Ask model directly before it's cleared by CustomCompletionLineEdit

    int outTrackCount = 0;
    if(gateId)
        outTrackCount = gatesMatchModel->getOutTrackCount(gateId);

    if(outTrackCount == 0)
        outTrackCount = 1000; //Set to some value
    gateTrackSpin->setMaximum(outTrackCount - 1);

    if(m_mode == GateToAllTracks)
    {
        //The side is deduced by gate side
        utils::Side gateSide = gatesMatchModel->getGateSide(gateId);
        trackSideCombo->setCurrentIndex(int(gateSide));
    }
}

void NewTrackConnDlg::onTrackSideChanged()
{
    if(m_mode == TrackToAllGates)
    {
        //Will connect to all gates on this side
        int idx = trackSideCombo->currentIndex();
        utils::Side side = utils::Side(idx);
        gateEdit->setPlaceholderText(tr("All Gates %1 Side")
                                         .arg(utils::StationUtils::name(side)));
    }
}
