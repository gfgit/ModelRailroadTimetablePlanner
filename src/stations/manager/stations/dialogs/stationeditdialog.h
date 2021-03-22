#ifndef STATIONEDITDIALOG_H
#define STATIONEDITDIALOG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
} // namespace sqlite3pp

class SpinBoxEditorFactory;
class StationTracksMatchFactory;

class StationGatesModel;
class StationTracksModel;

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

    void onGatesChanged();
    void addGate();
    void removeSelectedGate();

    void onTracksChanged();
    void addTrack();
    void removeSelectedTrack();
    void moveTrackUp();
    void moveTrackDown();

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

    SpinBoxEditorFactory *trackLengthSpinFactory;
    StationTracksMatchFactory *trackFactory;

    StationGatesModel *gatesModel;
    StationTracksModel *tracksModel;
};

#endif // STATIONEDITDIALOG_H
