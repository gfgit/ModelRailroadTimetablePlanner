#ifdef ENABLE_BACKGROUND_MANAGER

#include "jobcrossingchecker.h"

#include "jobcrossingtask.h"
#include "jobcrossingmodel.h"

JobCrossingChecker::JobCrossingChecker(sqlite3pp::database &db, QObject *parent) :
    IBackgroundChecker(db, parent)
{
    eventType = int(JobCrossingResultEvent::_Type);

    errorsModel = new JobCrossingModel(this);
}

QString JobCrossingChecker::getName() const
{
    return tr("Job Crossings");
}

void JobCrossingChecker::clearModel()
{
    static_cast<JobCrossingModel *>(errorsModel)->clear();
}

void JobCrossingChecker::showContextMenu(QWidget *panel, const QPoint &pos, const QModelIndex &idx) const
{
    //TODO
}

IQuittableTask *JobCrossingChecker::createMainWorker()
{
    return new JobCrossingTask(mDb, this, {});
}

void JobCrossingChecker::setErrors(QEvent *e, bool merge)
{
    auto model = static_cast<JobCrossingModel *>(errorsModel);
    auto ev = static_cast<JobCrossingResultEvent *>(e);
    if(merge)
        model->mergeErrors(ev->results);
    else
        model->setErrors(ev->results);
}

#endif // ENABLE_BACKGROUND_MANAGER
