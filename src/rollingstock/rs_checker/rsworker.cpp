#ifdef ENABLE_RS_CHECKER

#include "rsworker.h"

#include "utils/types.h"
#include "utils/rs_utils.h"

#include <QDebug>

#include "error_data.h"

#include <QVector>

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

RsErrWorker::RsErrWorker(database &db, QObject *receiver, QVector<db_id> vec) :
    IQuittableTask(receiver),
    mDb(db),
    rsToCheck(vec)
{
}

void RsErrWorker::run()
{
    using namespace RsErrors;

    QMap<db_id, RSErrorList> data;

    try{

        query q_selectCoupling(mDb, "SELECT coupling.id, coupling.operation, coupling.stop_id,"
                                    " stops.job_id, jobs.category,"
                                    " stops.station_id, stations.name,"
                                    " stops.type, stops.arrival, stops.departure"
                                    " FROM coupling"
                                    " JOIN stops ON stops.id=coupling.stop_id"
                                    " JOIN jobs ON jobs.id=stops.job_id"
                                    " JOIN stations ON stations.id=stops.station_id"
                                    " WHERE coupling.rs_id=? ORDER BY stops.arrival ASC");

        qDebug() << "Starting WORKER: rs check";

        if(rsToCheck.isEmpty())
        {
            query q_countRs(mDb, "SELECT COUNT() FROM rs_list");
            query q_selectRs(mDb, "SELECT rs_list.id,rs_list.number,"
                                  "rs_models.name,rs_models.suffix,rs_models.type"
                                  " FROM rs_list"
                                  " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id");

            q_countRs.step();
            int rsCount = q_countRs.getRows().get<int>(0);
            q_countRs.reset();

            int i = 0;
            sendEvent(new RsWorkerProgressEvent(0, rsCount), false);

            for(auto r : q_selectRs)
            {
                if(++i % 4 == 0) //Check every 4 RS to keep overhead low.
                {
                    if(wasStopped())
                        break;

                    sendEvent(new RsWorkerProgressEvent(i, rsCount), false);
                }

                RSErrorList rs;
                rs.rsId = r.get<db_id>(0);

                int number = r.get<int>(1);
                int modelNameLen = sqlite3_column_bytes(q_selectRs.stmt(), 2);
                const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(q_selectRs.stmt(), 2));

                int modelSuffixLen = sqlite3_column_bytes(q_selectRs.stmt(), 3);
                const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(q_selectRs.stmt(), 3));
                RsType type = RsType(r.get<int>(4));

                rs.rsName = rs_utils::formatNameRef(modelName, modelNameLen,
                                                    number,
                                                    modelSuffix, modelSuffixLen,
                                                    type);

                checkRs(rs, q_selectCoupling);

                if(rs.errors.size()) //Insert only if there are errors
                    data.insert(rs.rsId, rs);
            }
        }
        else
        {
            query q_getRsInfo(mDb, "SELECT rs_list.number,"
                                   "rs_models.name,rs_models.suffix,rs_models.type"
                                   " FROM rs_list"
                                   " LEFT JOIN rs_models ON rs_models.id=rs_list.model_id"
                                   " WHERE rs_list.id=?");
            int i = 0;
            for(db_id rsId : qAsConst(rsToCheck))
            {
                if(++i % 4 == 0 && wasStopped()) //Check every 4 RS to keep overhead low.
                    break;

                RSErrorList rs;
                rs.rsId = rsId;

                q_getRsInfo.bind(1, rs.rsId);
                if(q_getRsInfo.step() != SQLITE_ROW)
                {
                    q_getRsInfo.reset();
                    continue; //RS does not exist!
                }

                int number = sqlite3_column_int(q_getRsInfo.stmt(), 0);
                int modelNameLen = sqlite3_column_bytes(q_getRsInfo.stmt(), 1);
                const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(q_getRsInfo.stmt(), 1));

                int modelSuffixLen = sqlite3_column_bytes(q_getRsInfo.stmt(), 2);
                const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(q_getRsInfo.stmt(), 2));
                RsType type = RsType(sqlite3_column_int(q_getRsInfo.stmt(), 3));

                rs.rsName = rs_utils::formatNameRef(modelName, modelNameLen,
                                                    number,
                                                    modelSuffix, modelSuffixLen,
                                                    type);
                q_getRsInfo.reset();

                checkRs(rs, q_selectCoupling);

                //Insert also if there aren't errors to tell RsErrorTreeModel to remove this RS
                data.insert(rs.rsId, rs);
            }
        }

        sendEvent(new RsWorkerResultEvent(this, data, !rsToCheck.isEmpty()), true);
        return;
    }
    catch(std::exception e)
    {
        qWarning() << "RsErrWorker: exception " << e.what();
    }
    catch(...)
    {
        qWarning() << "RsErrWorker: generic exception";
    }

    //FIXME: Send error
    sendEvent(new RsWorkerResultEvent(this, data, !rsToCheck.isEmpty()), true);
}

void RsErrWorker::checkRs(RsErrors::RSErrorList &rs, query& q_selectCoupling)
{
    using namespace RsErrors;
    ErrorData err;
    err.rsId = rs.rsId;

    RsOp prevOp = RsOp::Uncoupled;

    db_id prevCouplingId = 0;
    db_id prevStopId = 0;
    db_id prevStation = 0;
    db_id prevJobId = 0;
    QTime prevTime;

    q_selectCoupling.bind(1, err.rsId);
    for(auto coup : q_selectCoupling)
    {
        err.couplingId = coup.get<db_id>(0);
        RsOp op = RsOp(coup.get<int>(1));
        err.stopId = coup.get<db_id>(2);
        err.jobId = coup.get<db_id>(3);
        err.jobCategory = JobCategory(coup.get<int>(4));
        err.stationId = coup.get<db_id>(5);
        err.stationName = coup.get<QString>(6);
        int transit = coup.get<int>(7);
        QTime arrival = coup.get<QTime>(8);
        //QTime departure = coup.get<QTime>(9); TODO: check departure less than next arrival

        err.time = arrival; //TODO: maybe arrival or departure depending

        err.otherId = prevCouplingId;

        if(op == prevOp)
        {
            if(op == RsOp::Coupled)
            {
                if(err.jobId != prevJobId && prevJobId != 0)
                {
                    //Rs was not uncoupled at the end of the job
                    //Or it was coupled by another job before prevJob uncouples it
                    //NOTE: this might be a false positive. Example below:
                    // 00:00 - Job 1 couples Rs
                    // 00:30 - Job 2 couples Rs
                    // --> here we detect 'Coupled twice' and 'Not uncoupled at end of job'
                    // 00:45 - Job 2 uncouples Rs
                    // 00:50 - Job 1 uncouples Rs
                    // --> here we detect 'Uncoupled when not coupled'
                    // because Job 2 already has uncoupled.
                    // But this also means that it's not true that Rs isn't uncoupled at end of the jobs


                    //Here we create another structure to fill it with previous data
                    ErrorData e;
                    e.couplingId = prevCouplingId;
                    e.rsId = rs.rsId;
                    e.stopId = prevStopId;
                    e.stationId = prevStation;
                    e.jobId = prevJobId;
                    e.otherId = e.couplingId;
                    e.time = prevTime;
                    e.errorType = NotUncoupledAtJobEnd;
                    rs.errors.append(e);
                }

                //Inform Rs was also coupled twice
                err.errorType = CoupledWhileBusy;

            }
            else
            {
                err.errorType = UncoupledWhenNotCoupled;
            }
            rs.errors.append(err);
        }

        if(transit)
        {
            err.errorType = StopIsTransit;
            rs.errors.append(err);
        }

        if(op == Coupled && prevOp == Uncoupled && err.stationId != prevStation && prevStation != 0)
        {
            err.errorType = CoupledInDifferentStation;
            rs.errors.append(err);
        }

        if(op == Uncoupled && prevOp == Coupled && err.jobId != prevJobId && prevJobId != 0)
        {
            err.errorType = UncoupledWhenNotCoupled;
            rs.errors.append(err);
        }

        if(err.stopId == prevStopId)
        {
            err.errorType = UncoupledInSameStop;
            rs.errors.append(err);
        }

        prevOp = op;
        prevCouplingId = err.couplingId;
        prevStopId = err.stopId;
        prevStation = err.stationId;
        prevJobId = err.jobId;
        prevTime = err.time;
    }
    q_selectCoupling.reset();

    if(prevOp == RsOp::Coupled)
    {
        err.errorType = NotUncoupledAtJobEnd;
        rs.errors.append(err);
    }
}

RsWorkerProgressEvent::RsWorkerProgressEvent(int pr, int max) :
    QEvent(_Type),
    progress(pr),
    progressMax(max)
{

}

RsWorkerProgressEvent::~RsWorkerProgressEvent()
{

}

RsWorkerResultEvent::RsWorkerResultEvent(RsErrWorker *worker, const QMap<db_id, RsErrors::RSErrorList> &data, bool merge) :
    QEvent(_Type),
    task(worker),
    results(data),
    mergeErrors(merge)
{

}

RsWorkerResultEvent::~RsWorkerResultEvent()
{

}

#endif // ENABLE_RS_CHECKER
