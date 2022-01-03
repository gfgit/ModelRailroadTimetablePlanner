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
}

class StationSVGJobStops;

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
    void setJobTime(const QTime& t);

private slots:
    void setZoom(int val);
    void zoomToFit();
    void onLabelClicked(qint64 gateId, QChar letter, const QString& text);
    void onTrackClicked(qint64 trackId, const QString& name);
    void onTrackConnClicked(qint64 connId, qint64 trackId, qint64 gateId,
                            int gateTrackPos, int trackSide);

    void startJobTimer();
    void stopJobTimer();
    void applyJobTime();

protected:
    void showEvent(QShowEvent *) override;
    void timerEvent(QTimerEvent *e) override;

private:
    sqlite3pp::database &mDb;
    db_id stationId;

    QToolBar *toolBar;
    QAction  *act_showJobs;
    QAction  *act_timeEdit;
    QTimeEdit *mTimeEdit;

    QScrollArea *scrollArea;
    ssplib::SSPViewer *view;

    QSvgRenderer *mSvg;
    ssplib::StationPlan *m_plan;

    StationSVGJobStops *m_station;
    QString mStationName;
    bool m_showJobs;
    int mJobTimer;

    int m_zoom;
};

#endif // STATIONSVGPLANDLG_H
