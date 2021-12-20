#ifndef STATIONSVGPLANDLG_H
#define STATIONSVGPLANDLG_H

#include <QWidget>

#include "utils/types.h"

class QIODevice;
class QSvgRenderer;

class QToolBar;
class QScrollArea;


namespace sqlite3pp {
class database;
}

namespace ssplib {
class StationPlan;
class SSPViewer;
}

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

    static bool stationHasSVG(sqlite3pp::database &db, db_id stId, QString *stNameOut);

signals:
    void zoomChanged(int zoom);

public slots:
    void reloadPlan();

private slots:
    void setZoom(int val);
    void zoomToFit();
    void onLabelClicked(qint64 gateId, QChar letter, const QString& text);

protected:
    void showEvent(QShowEvent *) override;

private:
    sqlite3pp::database &mDb;
    db_id stationId;

    QToolBar *toolBar;
    QScrollArea *scrollArea;
    ssplib::SSPViewer *view;

    QSvgRenderer *mSvg;
    ssplib::StationPlan *m_plan;

    int m_zoom;
};

#endif // STATIONSVGPLANDLG_H
