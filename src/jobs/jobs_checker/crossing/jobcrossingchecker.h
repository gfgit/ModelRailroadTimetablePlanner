#ifndef JOBCROSSINGCHECKER_H
#define JOBCROSSINGCHECKER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include "backgroundmanager/ibackgroundchecker.h"

#include "utils/types.h"

class JobCrossingChecker : public IBackgroundChecker
{
    Q_OBJECT
public:
    JobCrossingChecker(sqlite3pp::database &db, QObject *parent = nullptr);

    QString getName() const override;
    void clearModel() override;
    void showContextMenu(QWidget *panel, const QPoint& pos, const QModelIndex& idx) const override;

    void sessionLoadedHandler() override;

protected:
    IQuittableTask *createMainWorker() override;
    void setErrors(QEvent *e, bool merge) override;

private slots:
    void onJobChanged(db_id newJobId, db_id oldJobId);
    void onJobRemoved(db_id jobId);

private:
    sqlite3pp::database &mDb;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // JOBCROSSINGCHECKER_H
