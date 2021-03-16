#ifndef RSCOUPLEDIALOG_H
#define RSCOUPLEDIALOG_H

#include <QDialog>

#include "utils/types.h"

class RSCouplingInterface;
class RSProxyModel;
class QPushButton;

namespace sqlite3pp {
class database;
}

//FIXME: on-demand load and filter
class RSCoupleDialog : public QDialog
{
    Q_OBJECT
public:
    RSCoupleDialog(RSCouplingInterface *mgr, RsOp o, QWidget *parent = nullptr);

    void loadProxyModels(sqlite3pp::database &db, db_id jobId, db_id stopId, db_id stationId, const QTime &arrival);

private slots:
    void toggleLegend();

private:
    void updateButText();

private:
    RSCouplingInterface *couplingMgr;

    RSProxyModel *engModel;
    RSProxyModel *coachModel;
    RSProxyModel *freightModel;

    QPushButton *showHideLegendBut;
    QWidget *legend;

    RsOp op;
};

#endif // RSCOUPLEDIALOG_H
