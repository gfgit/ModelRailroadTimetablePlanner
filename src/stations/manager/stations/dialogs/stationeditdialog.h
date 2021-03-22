#ifndef STATIONEDITDIALOG_H
#define STATIONEDITDIALOG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
} // namespace sqlite3pp

namespace Ui {
class StationEditDialog;
}

class StationGatesModel;

class StationEditDialog : public QDialog
{
    Q_OBJECT

public:
    StationEditDialog(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~StationEditDialog();

    bool setStation(db_id stationId);
    db_id getStation() const;

public slots:
    void done(int res) override;

private slots:
    void modelError(const QString& msg);
    void currentTabChanged(int idx);

    void onGateNameChanged();
    void addGate();
    void removeSelectedGate();

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

    StationGatesModel *gatesModel;
};

#endif // STATIONEDITDIALOG_H
