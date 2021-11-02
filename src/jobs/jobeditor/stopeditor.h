#ifndef STOPEDITOR_H
#define STOPEDITOR_H

#include <QFrame>
#include <QTime>

#include "jobs/jobeditor/model/stopmodel.h" //TODO: include only StopItem

class CustomCompletionLineEdit;
class QTimeEdit;
class QGridLayout;

class StationsMatchModel;

class RailwaySegmentMatchModel;

namespace sqlite3pp {
class database;
}

class StopEditor : public QFrame
{
    Q_OBJECT
public:
    StopEditor(sqlite3pp::database &db, QWidget *parent = nullptr);

    void setStop(const StopItem& item, const StopItem& prev);

    inline const StopItem& getCurItem() const { return oldItem; }
    inline const StopItem& getPrevItem() const { return prevItem; }

private slots:
    void onStationSelected();

    void arrivalChanged(const QTime &arrival);

private:
    QGridLayout *lay;
    CustomCompletionLineEdit *mStationEdit;
    CustomCompletionLineEdit *mSegmentEdit;
    QTimeEdit *arrEdit;
    QTimeEdit *depEdit;

    StationsMatchModel *stationsMatchModel;
    RailwaySegmentMatchModel *segmentMatchModel;

    StopItem oldItem;
    StopItem prevItem;

    int prevSegmentRow;
};

#endif // STOPEDITOR_H
