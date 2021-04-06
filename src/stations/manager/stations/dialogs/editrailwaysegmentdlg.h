#ifndef EDITRAILWAYSEGMENTDLG_H
#define EDITRAILWAYSEGMENTDLG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class StationsMatchModel;
class StationGatesMatchModel;
class RailwaySegmentHelper;

class CustomCompletionLineEdit;
class QSpinBox;
class QCheckBox;
class QLineEdit;

//TODO: handle railway connections too!
class EditRailwaySegmentDlg : public QDialog
{
    Q_OBJECT
public:
    enum LockField
    {
        DoNotLock = 0,
        LockToCurrentValue = -1
    };

    EditRailwaySegmentDlg(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~EditRailwaySegmentDlg();

    virtual void done(int res) override;

    void setSegment(db_id segmentId, db_id lockStId, db_id lockGateId);

private slots:
    void onFromStationChanged(db_id stationId);
    void onToStationChanged(db_id stationId);

private:
    StationsMatchModel *fromStationMatch;
    StationGatesMatchModel *fromGateMatch;

    StationsMatchModel *toStationMatch;
    StationGatesMatchModel *toGateMatch;

    RailwaySegmentHelper *helper;

    CustomCompletionLineEdit *fromStationEdit;
    CustomCompletionLineEdit *fromGateEdit;

    CustomCompletionLineEdit *toStationEdit;
    CustomCompletionLineEdit *toGateEdit;

    QLineEdit *segmentNameEdit;
    QSpinBox *distanceSpin;
    QSpinBox *maxSpeedSpin;
    QCheckBox *electifiedCheck;

private:
    db_id m_segmentId;
    db_id m_lockStationId;
    db_id m_lockGateId;
    bool reversed;
};

#endif // EDITRAILWAYSEGMENTDLG_H
