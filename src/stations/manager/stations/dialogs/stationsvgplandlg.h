#ifndef STATIONSVGPLANDLG_H
#define STATIONSVGPLANDLG_H

#include <QDialog>

#include "utils/types.h"

class QIODevice;
class QSvgRenderer;

namespace sqlite3pp {
class database;
}

namespace ssplib {
class StationPlan;
class SSPViewer;
}

class StationSVGPlanDlg : public QDialog
{
    Q_OBJECT
public:
    explicit StationSVGPlanDlg(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~StationSVGPlanDlg();

    void setStation(db_id stId);
    void reloadSVG(QIODevice *dev);

private:
    sqlite3pp::database &mDb;
    db_id stationId;

    ssplib::SSPViewer *view;

    QSvgRenderer *mSvg;
    ssplib::StationPlan *m_plan;
};

#endif // STATIONSVGPLANDLG_H
