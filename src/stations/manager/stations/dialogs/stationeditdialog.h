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

private:
    void addTrackConnInternal(int mode);

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
};

#endif // STATIONEDITDIALOG_H
