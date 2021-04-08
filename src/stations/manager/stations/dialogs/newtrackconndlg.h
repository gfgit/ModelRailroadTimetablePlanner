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

//FIXME: add feature Add Track to All West/East gates
//       and Add Gate To all tracks (West/East side)
class NewTrackConnDlg : public QDialog
{
    Q_OBJECT
public:

    enum Mode
    {
        SingleConnection,
        TrackToAllGates,
        GateToAllTracks
    };

    NewTrackConnDlg(ISqlFKMatchModel *tracks,
                    StationGatesMatchModel *gates,
                    QWidget *parent = nullptr);

    virtual void done(int res) override;

    void getData(db_id &trackOut, utils::Side &trackSideOut,
                 db_id &gateOut, int &gateTrackOut);

    void setMode(Mode mode);

private slots:
    void onGateChanged(db_id gateId);
    void onTrackSideChanged();

private:
    ISqlFKMatchModel *trackMatchModel;
    StationGatesMatchModel *gatesMatchModel;

    CustomCompletionLineEdit *trackEdit;
    QComboBox *trackSideCombo;

    CustomCompletionLineEdit *gateEdit;
    QSpinBox *gateTrackSpin;

    Mode m_mode;
};

#endif // NEWTRACKCONNDLG_H
