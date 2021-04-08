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

class NewTrackConnDlg : public QDialog
{
    Q_OBJECT
public:
    NewTrackConnDlg(ISqlFKMatchModel *tracks,
                    StationGatesMatchModel *gates,
                    QWidget *parent = nullptr);

    virtual void done(int res) override;

    void getData(db_id &trackOut, utils::Side &trackSideOut,
                 db_id &gateOut, int &gateTrackOut);

private slots:
    void onGateChanged(db_id gateId);

private:
    ISqlFKMatchModel *trackMatchModel;
    StationGatesMatchModel *gatesMatchModel;

    CustomCompletionLineEdit *trackEdit;
    QComboBox *trackSideCombo;

    CustomCompletionLineEdit *gateEdit;
    QSpinBox *gateTrackSpin;
};

#endif // NEWTRACKCONNDLG_H
