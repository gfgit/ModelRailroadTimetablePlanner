#ifndef JOBCROSSINGCHECKER_H
#define JOBCROSSINGCHECKER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include "backgroundmanager/ibackgroundchecker.h"

class JobCrossingChecker : public IBackgroundChecker
{
public:
    JobCrossingChecker(sqlite3pp::database &db, QObject *parent = nullptr);

    QString getName() const override;
    void clearModel() override;
    void showContextMenu(QWidget *panel, const QPoint& pos, const QModelIndex& idx) const override;

protected:
    IQuittableTask *createMainWorker() override;
    void setErrors(QEvent *e, bool merge) override;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // JOBCROSSINGCHECKER_H
