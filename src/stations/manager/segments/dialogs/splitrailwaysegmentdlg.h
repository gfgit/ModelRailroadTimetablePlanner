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

class SplitRailwaySegmentDlg : public QDialog
{
    Q_OBJECT
public:
    SplitRailwaySegmentDlg(sqlite3pp::database &db, QWidget *parent = nullptr);

    void done(int res) override;

private slots:
    void selectSegment();
    void onStationSelected();
    void editNewSegment();

private:
    void setMainSegment(db_id segmentId);
    void setMiddleStation(db_id stationId);
    void setNewSegmentName(const QString& possibleName);

private:
    sqlite3pp::database &mDb;

    QLabel *segmentLabel;
    QPushButton *selectSegBut;

    QGroupBox *fromBox;
    QLabel *fromStationLabel;
    QLabel *fromGateLabel;

    QGroupBox *middleBox;
    CustomCompletionLineEdit *middleStationEdit;
    CustomCompletionLineEdit *middleInGateEdit;
    CustomCompletionLineEdit *middleOutGateEdit;
    QPushButton *editNewSegBut;

    QGroupBox *toBox;
    QLabel *toGateLabel;
    QLabel *toStationLabel;

    QDialogButtonBox *butBox;

    StationsMatchModel *stationsModel;
    StationGatesMatchModel *middleInGateModel;
    StationGatesMatchModel *middleOutGateModel;

    db_id mOriginalSegmentId;

    utils::RailwaySegmentGateInfo fromGate;
    utils::RailwaySegmentGateInfo middleInGate;
    utils::RailwaySegmentInfo newSegInfo;
};

#endif // SPLITRAILWAYSEGMENTDLG_H
