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
    ~SplitRailwaySegmentDlg();

    virtual void done(int res) override;

    void setMainSegment(db_id segmentId);

private slots:
    void selectSegment();
    void onStationSelected();
    void editNewSegment();

    void onMiddleInCompletionDone();
    void onMiddleOutCompletionDone();

private:
    void setMiddleStation(db_id stationId);
    void setNewSegmentName(const QString& possibleName);

private:
    sqlite3pp::database &mDb;

    //Original Segment
    QLabel *segmentLabel;
    QPushButton *selectSegBut;

    //From
    QGroupBox *fromBox;
    QLabel *fromStationLabel;
    QLabel *fromGateLabel;

    //Middle insert
    QGroupBox *middleBox;
    CustomCompletionLineEdit *middleStationEdit;
    CustomCompletionLineEdit *middleInGateEdit;
    CustomCompletionLineEdit *middleOutGateEdit;
    QPushButton *editNewSegBut;

    //To
    QGroupBox *toBox;
    QLabel *toGateLabel;
    QLabel *toStationLabel;

    QDialogButtonBox *butBox;

    StationsMatchModel *stationsModel;
    StationGatesMatchModel *middleInGateModel;
    StationGatesMatchModel *middleOutGateModel;

    utils::RailwaySegmentInfo origSegInfo;
    utils::RailwaySegmentInfo newSegInfo;
};

#endif // SPLITRAILWAYSEGMENTDLG_H
