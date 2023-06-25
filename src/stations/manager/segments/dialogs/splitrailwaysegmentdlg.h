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

#ifndef SPLITRAILWAYSEGMENTDLG_H
#define SPLITRAILWAYSEGMENTDLG_H

#include <QDialog>

#include "utils/types.h"
#include "stations/station_utils.h"

namespace sqlite3pp {
class database;
}

class QGroupBox;
class QLabel;
class QPushButton;
class QDialogButtonBox;

class CustomCompletionLineEdit;
class StationsMatchModel;
class StationGatesMatchModel;
class RailwaySegmentConnectionsModel;

class SplitRailwaySegmentDlg : public QDialog
{
    Q_OBJECT
public:
    SplitRailwaySegmentDlg(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~SplitRailwaySegmentDlg();

    virtual void done(int res) override;

    void setMainSegment(db_id segmentId);

private slots:
    void selectSegment();
    void onStationSelected();

    void editOldSegment();
    void editNewSegment();

    void onMiddleInCompletionDone();
    void onMiddleOutCompletionDone();

private:
    void setMiddleStation(db_id stationId);
    void setNewSegmentName(const QString &possibleName);

private:
    sqlite3pp::database &mDb;

    // Original Segment
    QLabel *segmentLabel;
    QPushButton *selectSegBut;

    // From
    QGroupBox *fromBox;
    QLabel *fromStationLabel;
    QLabel *fromGateLabel;
    QPushButton *editOldSegBut;

    // Middle insert
    QGroupBox *middleBox;
    CustomCompletionLineEdit *middleStationEdit;
    CustomCompletionLineEdit *middleInGateEdit;
    CustomCompletionLineEdit *middleOutGateEdit;
    QPushButton *editNewSegBut;

    // To
    QGroupBox *toBox;
    QLabel *toGateLabel;
    QLabel *toStationLabel;

    QDialogButtonBox *butBox;

    StationsMatchModel *stationsModel;
    StationGatesMatchModel *middleInGateModel;
    StationGatesMatchModel *middleOutGateModel;

    RailwaySegmentConnectionsModel *origSegConnModel;
    RailwaySegmentConnectionsModel *newSegConnModel;

    utils::RailwaySegmentInfo origSegInfo;
    utils::RailwaySegmentInfo newSegInfo;
};

#endif // SPLITRAILWAYSEGMENTDLG_H
