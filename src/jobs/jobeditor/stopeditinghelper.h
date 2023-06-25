/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef STOPEDITINGHELPER_H
#define STOPEDITINGHELPER_H

#include <QObject>

#include "jobs/jobeditor/model/stopmodel.h"

class QWidget;
class CustomCompletionLineEdit;
class QTimeEdit;
class QSpinBox;

class StationsMatchModel;
class StationTracksMatchModel;
class StationGatesMatchModel;

class QModelIndex;

namespace sqlite3pp {
class database;
}

class StopEditingHelper : public QObject
{
    Q_OBJECT
public:
    StopEditingHelper(sqlite3pp::database &db, StopModel *m, QSpinBox *outTrackSpin, QTimeEdit *arr,
                      QTimeEdit *dep, QWidget *editor = nullptr);
    ~StopEditingHelper();

    void setStop(const StopItem &item, const StopItem &prev);

    void popupSegmentCombo();

    QString getGateString(db_id gateId, bool reversed);

    inline CustomCompletionLineEdit *getStationEdit() const
    {
        return mStationEdit;
    }
    inline CustomCompletionLineEdit *getStTrackEdit() const
    {
        return mStTrackEdit;
    }
    inline CustomCompletionLineEdit *getOutGateEdit() const
    {
        return mOutGateEdit;
    }

    inline const StopItem &getCurItem() const
    {
        return curStop;
    }
    inline const StopItem &getPrevItem() const
    {
        return prevStop;
    }

protected:
    void timerEvent(QTimerEvent *e) override;

signals:
    void nextSegmentChosen();
    void stationTrackChosen();

private slots:
    void onStationSelected();
    void onTrackSelected();
    void onOutGateSelected(const QModelIndex &idx);
    void checkOutGateTrack();

    void arrivalChanged(const QTime &arrival);
    void departureChanged(const QTime &dep);

public slots:
    void startOutTrackTimer();
    void stopOutTrackTimer();

private:
    void updateGateTrackSpin(const StopItem::Gate &toGate);

private:
    QWidget *mEditor;

    CustomCompletionLineEdit *mStationEdit;
    CustomCompletionLineEdit *mStTrackEdit;
    CustomCompletionLineEdit *mOutGateEdit;
    QTimeEdit *arrEdit;
    QTimeEdit *depEdit;
    QSpinBox *mOutGateTrackSpin;

    StationsMatchModel *stationsMatchModel;
    StationTracksMatchModel *stationTrackMatchModel;
    StationGatesMatchModel *stationOutGateMatchModel;

    StopModel *model;
    StopItem curStop;
    StopItem prevStop;

    int mTimerOutTrack;
};

#endif // STOPEDITINGHELPER_H
