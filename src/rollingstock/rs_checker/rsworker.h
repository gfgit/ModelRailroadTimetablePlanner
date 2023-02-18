#ifndef RSWORKER_H
#define RSWORKER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include "utils/thread/iquittabletask.h"
#include "utils/thread/taskprogressevent.h"

#include <QMap>
#include <QVector>

#include "rs_error_data.h"


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

class RsWorkerResultEvent : public GenericTaskEvent
{
public:
    static const Type _Type = Type(CustomEvents::RsErrWorkerResult);

    RsWorkerResultEvent(RsErrWorker *worker, const QMap<db_id, RsErrors::RSErrorList> &data, bool merge);
    ~RsWorkerResultEvent();

    QMap<db_id, RsErrors::RSErrorList> results;
    bool mergeErrors;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // RSWORKER_H
