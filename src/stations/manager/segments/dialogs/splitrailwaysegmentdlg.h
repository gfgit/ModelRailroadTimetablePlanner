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

class CustomCompletionLineEdit;
class StationsMatchModel;
class StationGatesMatchModel;

class SplitRailwaySegmentDlg : public QDialog
{
    Q_OBJECT
public:
    SplitRailwaySegmentDlg(sqlite3pp::database &db, QWidget *parent = nullptr);

private slots:
    void selectSegment();
    void onStationSelected();

private:
    void setMainSegment(db_id segmentId);
    void setMiddleStation(db_id stationId);

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

    QGroupBox *toBox;
    QLabel *toGateLabel;
    QLabel *toStationLabel;

    StationsMatchModel *stationsModel;
    StationGatesMatchModel *middleInGateModel;
    StationGatesMatchModel *middleOutGateModel;

    db_id mOriginalSegmentId;
    utils::RailwaySegmentType segmentType;

    struct Gate
    {
        db_id gateId = 0;
        db_id stationId = 0;
        QChar gateLetter;
    };

    Gate fromGate;
    Gate middleInGate;
    Gate middleOutGate;
    Gate toGate;
};

#endif // SPLITRAILWAYSEGMENTDLG_H
