#ifndef RSCHECKERMANAGER_H
#define RSCHECKERMANAGER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include "backgroundmanager/ibackgroundchecker.h"

#include "utils/types.h"

class RsCheckerManager : public IBackgroundChecker
{
    Q_OBJECT
public:
    RsCheckerManager(sqlite3pp::database &db, QObject *parent = nullptr);

    void checkRs(const QSet<db_id> &rsIds);

    QString getName() const override;
    void clearModel() override;
    void showContextMenu(QWidget *panel, const QPoint& pos, const QModelIndex& idx) const override;

public slots:
    void onRSPlanChanged(const QSet<db_id> &rsIds);

protected:
    IQuittableTask *createMainWorker() override;
    void setErrors(QEvent *e, bool merge) override;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // RSCHECKERMANAGER_H
