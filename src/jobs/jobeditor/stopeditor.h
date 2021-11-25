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
class RailwaySegmentMatchModel;

namespace sqlite3pp {
class database;
}

class StopEditor : public QFrame
{
    Q_OBJECT
public:
    StopEditor(sqlite3pp::database &db, StopModel *m, QWidget *parent = nullptr);

    void setStop(const StopItem& item, const StopItem& prev);

    inline const StopItem& getCurItem() const { return oldItem; }
    inline const StopItem& getPrevItem() const { return prevItem; }

private slots:
    void onStationSelected();
    void onTrackSelected();
    void onNextSegmentSelected();

    void arrivalChanged(const QTime &arrival);

private:
    QGridLayout *lay;
    CustomCompletionLineEdit *mStationEdit;
    CustomCompletionLineEdit *mTrackEdit;
    CustomCompletionLineEdit *mSegmentEdit;
    QTimeEdit *arrEdit;
    QTimeEdit *depEdit;

    StationsMatchModel *stationsMatchModel;
    StationTracksMatchModel *stationTrackMatchModel;
    RailwaySegmentMatchModel *segmentMatchModel;

    StopModel *model;
    StopItem oldItem;
    StopItem prevItem;
};

#endif // STOPEDITOR_H
