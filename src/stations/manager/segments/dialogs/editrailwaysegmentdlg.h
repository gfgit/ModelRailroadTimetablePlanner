#ifndef EDITRAILWAYSEGMENTDLG_H
#define EDITRAILWAYSEGMENTDLG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

namespace utils {
struct RailwaySegmentInfo;
}

class StationsMatchModel;
class StationGatesMatchModel;
class RailwaySegmentHelper;
class RailwaySegmentConnectionsModel;

class QGroupBox;
class CustomCompletionLineEdit;
class QSpinBox;
class QCheckBox;
class QLineEdit;


class EditRailwaySegmentDlg : public QDialog
{
    Q_OBJECT
public:
    enum LockField
    {
        DoNotLock = 0,
        LockToCurrentValue = -1
    };

    /*!
     * \brief EditRailwaySegmentDlg
     * \param db database connection
     * \param conn segment connection model
     * \param parent parent widget
     *
     * If \ref conn is nullptr then a default one is created and destroyed
     * when the dialog gets destroyed.
     * Otherwise the model passed is used, user actions will be saved on it.
     * Caller is responsible to free the passed connection model.
     *
     * It's useful with \ref setManuallyApply() so changes can be recordes
     * for later use.
     *
     * \sa RailwaySegmentConnectionsModel
     */
    EditRailwaySegmentDlg(sqlite3pp::database &db,
                          RailwaySegmentConnectionsModel *conn = nullptr,
                          QWidget *parent = nullptr);
    ~EditRailwaySegmentDlg();

    virtual void done(int res) override;

    void setSegment(db_id segmentId, db_id lockStId, db_id lockGateId);
    void setSegmentInfo(const utils::RailwaySegmentInfo& info);

    void setGatesReadOnly(bool val);

    bool checkValues();
    bool applyChanges();

    bool fillSegInfo(utils::RailwaySegmentInfo &info);

    /*!
     * \brief setManuallyApply
     * \param val true to manually apply changes
     *
     * If false, changes will be applyed automatically when
     * dialog is accepted (closed with 'Ok' button).
     * Otherwise, caller is responsible to apply them later
     * or discarding them.
     *
     * \sa fillSegInfo()
     */
    void setManuallyApply(bool val);

private slots:
    void onFromStationChanged(db_id stationId);
    void onToStationChanged(db_id stationId);
    void updateTrackConnectionModel();
    void editSegmentTrackConnections();

private:
    StationsMatchModel *fromStationMatch;
    StationGatesMatchModel *fromGateMatch;

    StationsMatchModel *toStationMatch;
    StationGatesMatchModel *toGateMatch;

    RailwaySegmentHelper *helper;
    RailwaySegmentConnectionsModel *connModel;

    QGroupBox *fromBox;
    QGroupBox *toBox;

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
    bool manuallyApply;
};

#endif // EDITRAILWAYSEGMENTDLG_H
