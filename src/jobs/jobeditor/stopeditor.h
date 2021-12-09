#ifndef STOPEDITOR_H
#define STOPEDITOR_H

#include <QFrame>
#include <QTime>

#include "jobs/jobeditor/model/stopmodel.h"

class CustomCompletionLineEdit;
class QTimeEdit;
class QGridLayout;

class StationsMatchModel;
class StationTracksMatchModel;
class StationGatesMatchModel;

class QModelIndex;

namespace sqlite3pp {
class database;
}

/*!
 * \brief The StopEditor class
 *
 * Widget to edit job stops inside JobPathEditor
 *
 * \sa JobPathEditor
 * \sa StopModel
 */
class StopEditor : public QFrame
{
    Q_OBJECT
public:
    StopEditor(sqlite3pp::database &db, StopModel *m, QWidget *parent = nullptr);

    void setStop(const StopItem& item, const StopItem& prev);

    void updateStopArrDep();

    inline const StopItem& getCurItem() const { return curStop; }
    inline const StopItem& getPrevItem() const { return prevStop; }

    /*!
     * \brief closeOnSegmentChosen
     * \return true if editor should be closed after user has chosen a valid next segment
     */
    inline bool closeOnSegmentChosen() const { return m_closeOnSegmentChosen; };
    void setCloseOnSegmentChosen(bool value);

signals:
    /*!
     * \brief nextSegmentChosen
     * \param ed self instance
     *
     * Emitted when user has chosen next segment
     * And it is succesfully applied (it passes checks)
     */
    void nextSegmentChosen(StopEditor *ed);

public slots:
    /*!
     * \brief Popup next segment combo
     *
     * Popoup suggestions if there are multiple opptions available
     * If only one segment can be set as next segment, choose it automatically
     * and close editor.
     * This is done because QAbstractItemView::edit() does not let you pass additional arguments
     *
     * \sa nextSegmentChosen()
     */
    void popupSegmentCombo();

private slots:
    void onStationSelected();
    void onTrackSelected();
    void onOutGateSelected(const QModelIndex &idx);

    void arrivalChanged(const QTime &arrival);

private:
    QGridLayout *lay;
    CustomCompletionLineEdit *mStationEdit;
    CustomCompletionLineEdit *mStTrackEdit;
    CustomCompletionLineEdit *mOutGateEdit;
    QTimeEdit *arrEdit;
    QTimeEdit *depEdit;

    StationsMatchModel *stationsMatchModel;
    StationTracksMatchModel *stationTrackMatchModel;
    StationGatesMatchModel *stationOutGateMatchModel;

    StopModel *model;
    StopItem curStop;
    StopItem prevStop;

    bool m_closeOnSegmentChosen;
};

#endif // STOPEDITOR_H
