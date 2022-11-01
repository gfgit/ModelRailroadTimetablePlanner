#ifndef RSWORKER_H
#define RSWORKER_H

#ifdef ENABLE_RS_CHECKER

#ifndef ENABLE_BACKGROUND_MANAGER
#error "Cannot use ENABLE_RS_CHECKER without ENABLE_BACKGROUND_MANAGER"
#endif

#include "utils/thread/iquittabletask.h"

#include <QMap>
#include <QVector>

#include "utils/worker_event_types.h"

#include "error_data.h"


namespace sqlite3pp
{
class database;
class query;
}

class RsErrWorker : public IQuittableTask
{
public:
    RsErrWorker(sqlite3pp::database& db, QObject *receiver, const QVector<db_id>& vec);

    void run() override;

private:
    void checkRs(RsErrors::RSErrorList &rs, sqlite3pp::query &q_selectCoupling);
    void finish(const QMap<db_id, RsErrors::RSErrorList> &results, bool merge);

private:
    sqlite3pp::database &mDb;

    QVector<db_id> rsToCheck;
};

class RsWorkerProgressEvent : public QEvent
{
public:
    static const Type _Type = Type(CustomEvents::RsErrWorkerProgress);

    RsWorkerProgressEvent(int pr, int max);
    ~RsWorkerProgressEvent();

    int progress;
    int progressMax;
};

class RsWorkerResultEvent : public QEvent
{
public:
    static const Type _Type = Type(CustomEvents::RsErrWorkerResult);

    RsWorkerResultEvent(RsErrWorker *worker, const QMap<db_id, RsErrors::RSErrorList> &data, bool merge);
    ~RsWorkerResultEvent();

    RsErrWorker *task;
    QMap<db_id, RsErrors::RSErrorList> results;
    bool mergeErrors;
};

#endif // ENABLE_RS_CHECKER

#endif // RSWORKER_H
