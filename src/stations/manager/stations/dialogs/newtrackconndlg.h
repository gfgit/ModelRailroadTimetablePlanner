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

#ifndef NEWTRACKCONNDLG_H
#define NEWTRACKCONNDLG_H

#include <QDialog>

#include "utils/types.h"
#include "stations/station_utils.h"

class ISqlFKMatchModel;
class StationGatesMatchModel;

class CustomCompletionLineEdit;
class QComboBox;
class QSpinBox;

// FIXME: add feature Add Track to All West/East gates
//        and Add Gate To all tracks (West/East side)
class NewTrackConnDlg : public QDialog
{
    Q_OBJECT
public:
    enum Mode
    {
        SingleConnection,
        TrackToAllGates,
        GateToAllTracks
    };

    NewTrackConnDlg(ISqlFKMatchModel *tracks, StationGatesMatchModel *gates,
                    QWidget *parent = nullptr);

    virtual void done(int res) override;

    void getData(db_id &trackOut, utils::Side &trackSideOut, db_id &gateOut, int &gateTrackOut);

    void setMode(Mode mode);

private slots:
    void onGateChanged(db_id gateId);
    void onTrackSideChanged();

private:
    ISqlFKMatchModel *trackMatchModel;
    StationGatesMatchModel *gatesMatchModel;

    CustomCompletionLineEdit *trackEdit;
    QComboBox *trackSideCombo;

    CustomCompletionLineEdit *gateEdit;
    QSpinBox *gateTrackSpin;

    Mode m_mode;
};

#endif // NEWTRACKCONNDLG_H
