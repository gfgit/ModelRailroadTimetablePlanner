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

#ifndef STATIONSVGPLANDLG_H
#define STATIONSVGPLANDLG_H

#include <QWidget>

#include "utils/types.h"

class QIODevice;
class QSvgRenderer;

class QToolBar;
class QScrollArea;
class QTimeEdit;

namespace sqlite3pp {
class database;
}

namespace ssplib {
class StationPlan;
class SSPViewer;
} // namespace ssplib

struct StationSVGJobStops;

class StationSVGPlanDlg : public QWidget
{
    Q_OBJECT
public:
    explicit StationSVGPlanDlg(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~StationSVGPlanDlg();

    void setStation(db_id stId);
    void reloadSVG(QIODevice *dev);
    void reloadDBData();
    void clearDBData();

    void clearJobs();
    void reloadJobs();

    static bool stationHasSVG(sqlite3pp::database &db, db_id stId, QString *stNameOut);

signals:
    void zoomChanged(int zoom);

public slots:
    void reloadPlan();
    void showJobs(bool val);
    void setJobTime(const QTime &t);

private slots:
    void setZoom(int val, bool force = false);
    void setZoom_slot(int val);
    void zoomToFit();
    void onLabelClicked(qint64 gateId, QChar letter, const QString &text);
    void onTrackClicked(qint64 trackId, const QString &name);
    void onTrackConnClicked(qint64 connId, qint64 trackId, qint64 gateId, int gateTrackPos,
                            int trackSide);

    void startJobTimer();
    void stopJobTimer();
    void applyJobTime();

    void goToPrevStop();
    void goToNextStop();

protected:
    void showEvent(QShowEvent *) override;
    void timerEvent(QTimerEvent *e) override;

private:
    void clearJobs_internal();

private:
    sqlite3pp::database &mDb;
    db_id stationId;

    QToolBar *toolBar;
    QAction *act_showJobs;
    QAction *act_timeEdit;
    QAction *act_prevTime;
    QAction *act_nextTime;
    QTimeEdit *mTimeEdit;

    QScrollArea *scrollArea;
    ssplib::SSPViewer *view;

    QSvgRenderer *mSvg;
    ssplib::StationPlan *m_plan;

    StationSVGJobStops *m_station;
    bool m_showJobs;
    int mJobTimer;

    int m_zoom;
};

#endif // STATIONSVGPLANDLG_H
