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

#ifndef STATIONEDITDIALOG_H
#define STATIONEDITDIALOG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
} // namespace sqlite3pp

class SpinBoxEditorFactory;
class StationTracksMatchFactory;
class StationGatesMatchFactory;
class RailwaySegmentsModel;

class StationGatesModel;
class StationTracksModel;
class StationTrackConnectionsModel;

namespace Ui {
class StationEditDialog;
}

class StationEditDialog : public QDialog
{
    Q_OBJECT

public:
    StationEditDialog(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~StationEditDialog();

    bool setStation(db_id stationId);
    db_id getStation() const;

    void setStationInternalEditingEnabled(bool enable);
    void setStationExternalEditingEnabled(bool enable);
    void setGateConnectionsVisible(bool enable);

public slots:
    void done(int res) override;

private slots:
    void modelError(const QString& msg);

    //Gates
    void onGatesChanged();
    void addGate();
    void removeSelectedGate();

    //Tracks
    void onTracksChanged();
    void addTrack();
    void removeSelectedTrack();
    void moveTrackUp();
    void moveTrackDown();

    //Track Connections
    void onTrackConnRemoved();
    void removeSelectedTrackConn();

    //Gate Connections
    void addGateConnection();
    void editGateConnection();
    void removeSelectedGateConnection();

    //SVG Image
    void addSVGImage();
    void removeSVGImage();
    void saveSVGToFile();
    void importConnFromSVG();

    //Xml Plan
    void saveXmlPlan();

private:
    void addTrackConnInternal(int mode);
    void updateSVGButtons(bool hasImage);

private:
    enum Tabs
    {
        StationTab = 0,
        GatesTab,
        TracksTab,
        TrackConnectionsTab,
        GateConnectionsTab,
        NTabs
    };

    Ui::StationEditDialog *ui;

    sqlite3pp::database &mDb;

    SpinBoxEditorFactory *trackLengthSpinFactory;
    StationTracksMatchFactory *trackFactory;
    StationGatesMatchFactory *gatesFactory;

    StationGatesModel *gatesModel;
    StationTracksModel *tracksModel;
    StationTrackConnectionsModel *trackConnModel;
    RailwaySegmentsModel *gateConnModel;

    bool mEnableInternalEdititing;
};

#endif // STATIONEDITDIALOG_H
