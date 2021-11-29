#include "rscouplinginterface.h"

#include <QMessageBox>

#include <QApplication>

#include <QDebug>

#include "stopmodel.h"

RSCouplingInterface::RSCouplingInterface(database &db, QObject *parent) :
    QObject(parent),
    stopsModel(nullptr),
    mDb(db),
    q_deleteCoupling(mDb, "DELETE FROM coupling WHERE stop_id=? AND rs_id=?"),
    q_addCoupling(mDb, "INSERT INTO"
                       " coupling(stop_id,rs_id,operation)"
                       " VALUES(?, ?, ?)")
{

}

void RSCouplingInterface::loadCouplings(StopModel *model, db_id stopId, db_id jobId, QTime arr)
{
    stopsModel = model;

    m_stopId = stopId;
    m_jobId = jobId;
    arrival = arr;

    coupled.clear();
    uncoupled.clear();

    query q(mDb, "SELECT rs_id, operation FROM coupling WHERE stop_id=?");
    q.bind(1, m_stopId);

    for(auto rs : q)
    {
        db_id rsId = rs.get<db_id>(0);
        int op = rs.get<int>(1);

        if(op == RsOp::Coupled)
            coupled.append(rsId);
        else
            uncoupled.append(rsId);
    }
}

bool RSCouplingInterface::contains(db_id rsId, RsOp op) const
{
    if(op == RsOp::Coupled)
        return coupled.contains(rsId);
    else
        return uncoupled.contains(rsId);
}

bool RSCouplingInterface::coupleRS(db_id rsId, const QString& rsName, bool on, bool checkTractionType)
{
    stopsModel->startStopsEditing();
    stopsModel->markRsToUpdate(rsId);

    if(on)
    {
        if(coupled.contains(rsId))
        {
            qWarning() << "Error already checked:" << rsId;
            return true;
        }

        db_id jobId = 0;

        query q_RS_lastOp(mDb, "SELECT MAX(stops.arrival), coupling.operation, stops.job_id"
                               " FROM stops"
                               " JOIN coupling"
                               " ON coupling.stop_id=stops.id"
                               " AND coupling.rs_id=?"
                               " AND stops.arrival<?");
        q_RS_lastOp.bind(1, rsId);
        q_RS_lastOp.bind(2, arrival);
        int ret = q_RS_lastOp.step();

        bool isOccupied = false; //No Op means RS is turned off in a depot so it isn't occupied
        if(ret == SQLITE_ROW)
        {
            auto row = q_RS_lastOp.getRows();
            RsOp operation = RsOp(row.get<int>(1)); //Get last operation
            jobId = row.get<db_id>(2);
            isOccupied = (operation == RsOp::Coupled);
        }

        if(isOccupied)
        {
            if(jobId == m_jobId)
            {
                qWarning() << "Error while adding coupling op. Stop:" << m_stopId
                           << "Rs:" << rsId << "Already coupled by this job:" << m_jobId;

                QMessageBox::warning(qApp->activeWindow(),
                                     tr("Error"),
                                     tr("Error while adding coupling operation.\n"
                                        "Rollingstock %1 is already coupled by this job (%2)")
                                     .arg(rsName).arg(m_jobId),
                                     QMessageBox::Ok);
                return false;
            }
            else
            {
                qWarning() << "Error while adding coupling op. Stop:" << m_stopId
                           << "Rs:" << rsId << "Occupied by this job:" << jobId;

                int but = QMessageBox::warning(qApp->activeWindow(),
                                               tr("Error"),
                                               tr("Error while adding coupling operation.\n"
                                                  "Rollingstock %1 is already coupled to another job (%2)\n"
                                                  "Do you still want to couple it?")
                                               .arg(rsName).arg(jobId),
                                               QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

                if(but == QMessageBox::No)
                    return false; //Abort
            }
        }

        if(checkTractionType)
        {
            LineType lineType = stopsModel->getLineTypeAfterStop(m_stopId);
            if(lineType == LineType::NonElectric)
            {
                //Query RS type
                query q_getRSType(mDb, "SELECT rs_models.type,rs_models.sub_type"
                                       " FROM rs_list"
                                       " JOIN rs_models ON rs_models.id=rs_list.model_id"
                                       " WHERE rs_list.id=?");
                q_getRSType.bind(1, rsId);
                if(q_getRSType.step() != SQLITE_ROW)
                {
                    qWarning() << "RS seems to not exist, ID:" << rsId;
                }

                auto rs = q_getRSType.getRows();
                RsType type = RsType(rs.get<int>(0));
                RsEngineSubType subType = RsEngineSubType(rs.get<int>(1));

                if(type == RsType::Engine && subType == RsEngineSubType::Electric)
                {
                    int but = QMessageBox::warning(qApp->activeWindow(),
                                                   tr("Warning"),
                                                   tr("Rollingstock %1 is an Electric engine but the line is not electrified\n"
                                                      "This engine will not be albe to move a train.\n"
                                                      "Do you still want to couple it?")
                                                   .arg(rsName),
                                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                    if(but == QMessageBox::No)
                        return false; //Abort
                }
            }
        }

        q_addCoupling.bind(1, m_stopId);
        q_addCoupling.bind(2, rsId);
        q_addCoupling.bind(3, RsOp::Coupled);
        ret = q_addCoupling.execute();
        q_addCoupling.reset();

        if(ret != SQLITE_OK)
        {
            qWarning() << "Error while adding coupling op. Stop:" << m_stopId
                       << "Rs:" << rsId << "Op: Coupled " << "Ret:" << ret
                       << mDb.error_msg();
            return false;
        }

        coupled.append(rsId);

        //Check if there is a next coupling operation in the same job
        query q(mDb, "SELECT s2.id, s2.arrival, s2.stationId, stations.name"
                     " FROM coupling"
                     " JOIN stops s2 ON s2.id=coupling.stop_id"
                     " JOIN stops s1 ON s1.id=?"
                     " JOIN stations ON stations.id=s2.station_id"
                     " WHERE coupling.rs_id=? AND coupling.operation=? AND s1.job_id=s2.job_id AND s1.arrival < s2.arrival");
        q.bind(1, m_stopId);
        q.bind(2, rsId);
        q.bind(3, RsOp::Coupled);

        if(q.step() == SQLITE_ROW)
        {
            auto r = q.getRows();
            db_id stopId = r.get<db_id>(0);
            QTime arr = r.get<QTime>(1);
            db_id stId = r.get<db_id>(2);
            QString stName = r.get<QString>(3);

            qDebug() << "Found coupling, RS:" << rsId << "Stop:" << stopId << "St:" << stId << arr;

            int but = QMessageBox::question(qApp->activeWindow(),
                                            tr("Delete coupling?"),
                                            tr("You couple %1 also in a next stop in %2 at %3.\n"
                                               "Do you want to remove the other coupling operation?")
                                            .arg(rsName)
                                            .arg(stName)
                                            .arg(arr.toString("HH:mm")),
                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if(but == QMessageBox::Yes)
            {
                qDebug() << "Deleting coupling";

                q_deleteCoupling.bind(1, stopId);
                q_deleteCoupling.bind(2, rsId);
                ret = q_deleteCoupling.execute();
                q_deleteCoupling.reset();

                if(ret != SQLITE_OK)
                {
                    qWarning() << "Error while deleting next coupling op. Stop:" << stopId
                               << "Rs:" << rsId << "Op: Uncoupled " << "Ret:" << ret
                               << mDb.error_msg();
                }
            }
            else
            {
                qDebug() << "Keeping couple";
            }
        }
    }
    else
    {
        int row = coupled.indexOf(rsId);
        if(row == -1)
            return false;

        q_deleteCoupling.bind(1, m_stopId);
        q_deleteCoupling.bind(2, rsId);
        int ret = q_deleteCoupling.execute();
        q_deleteCoupling.reset();

        if(ret != SQLITE_OK)
        {
            qWarning() << "Error while deleting coupling op. Stop:" << m_stopId
                       << "Rs:" << rsId << "Op: Coupled " << "Ret:" << ret
                       << mDb.error_msg();
            return false;
        }

        coupled.removeAt(row);

        //Check if there is a next uncoupling operation
        query q(mDb, "SELECT s2.id, MIN(s2.arrival), s2.station_id, stations.name"
                     " FROM coupling"
                     " JOIN stops s2 ON s2.id=coupling.stop_id"
                     " JOIN stops s1 ON s1.id=?"
                     " JOIN stations ON stations.id=s2.station_id"
                     " WHERE coupling.rs_id=? AND coupling.operation=? AND s2.arrival > s1.arrival AND s2.job_id=s1.job_id");
        q.bind(1, m_stopId);
        q.bind(2, rsId);
        q.bind(3, RsOp::Uncoupled);

        if(q.step() == SQLITE_ROW && q.getRows().column_type(0) != SQLITE_NULL)
        {
            auto r = q.getRows();
            db_id stopId = r.get<db_id>(0);
            QTime arr = r.get<QTime>(1);
            db_id stId = r.get<db_id>(2);
            QString stName = r.get<QString>(3);

            qDebug() << "Found uncoupling, RS:" << rsId << "Stop:" << stopId << "St:" << stId << arr;

            int but = QMessageBox::question(qApp->activeWindow(),
                                            tr("Delete uncoupling?"),
                                            tr("You don't couple %1 anymore.\n"
                                               "Do you want to remove also the uncoupling operation in %2 at %3?")
                                            .arg(rsName)
                                            .arg(stName)
                                            .arg(arr.toString("HH:mm")),
                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if(but == QMessageBox::Yes)
            {
                qDebug() << "Deleting coupling";

                q_deleteCoupling.bind(1, stopId);
                q_deleteCoupling.bind(2, rsId);
                ret = q_deleteCoupling.execute();
                q_deleteCoupling.reset();

                if(ret != SQLITE_OK)
                {
                    qWarning() << "Error while deleting next uncoupling op. Stop:" << stopId
                               << "Rs:" << rsId << "Op: Uncoupled " << "Ret:" << ret
                               << mDb.error_msg();
                }
            }
            else
            {
                qDebug() << "Keeping couple";
            }
        }
    }

    return true;
}

bool RSCouplingInterface::uncoupleRS(db_id rsId, const QString& rsName, bool on)
{
    stopsModel->startStopsEditing();
    stopsModel->markRsToUpdate(rsId);

    if(on)
    {
        if(uncoupled.contains(rsId))
        {
            qWarning() << "Error already checked:" << rsId;
            return true;
        }

        q_addCoupling.bind(1, m_stopId);
        q_addCoupling.bind(2, rsId);
        q_addCoupling.bind(3, RsOp::Uncoupled);
        int ret = q_addCoupling.execute();
        q_addCoupling.reset();

        if(ret != SQLITE_OK)
        {
            qWarning() << "Error while adding coupling op. Stop:" << m_stopId
                       << "Rs:" << rsId << "Op: Uncoupled " << "Ret:" << ret
                       << mDb.error_msg();
            return false;
        }

        uncoupled.append(rsId);

        //Check if there is a next uncoupling operation
        query q(mDb, "SELECT s2.id, MIN(s2.arrival), s2.station_id, stations.name"
                     " FROM coupling"
                     " JOIN stops s2 ON s2.id=coupling.stop_id"
                     " JOIN stops s1 ON s1.id=?"
                     " JOIN stations ON stations.id=s2.station_id"
                     " WHERE coupling.rs_id=? AND coupling.operation=? AND s2.arrival > s1.arrival AND s2.job_id=s1.job_id");
        q.bind(1, m_stopId);
        q.bind(2, rsId);
        q.bind(3, RsOp::Uncoupled);

        if(q.step() == SQLITE_ROW && q.getRows().column_type(0) != SQLITE_NULL)
        {
            auto r = q.getRows();
            db_id stopId = r.get<db_id>(0);
            QTime arr = r.get<QTime>(1);
            db_id stId = r.get<db_id>(2);
            QString stName = r.get<QString>(3);

            qDebug() << "Found uncoupling, RS:" << rsId << "Stop:" << stopId << "St:" << stId << arr;

            int but = QMessageBox::question(qApp->activeWindow(),
                                            tr("Delete uncoupling?"),
                                            tr("You uncouple %1 also in %2 at %3.\n"
                                               "Do you want to remove the other uncoupling operation?")
                                            .arg(rsName)
                                            .arg(stName)
                                            .arg(arr.toString("HH:mm")),
                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if(but == QMessageBox::Yes)
            {
                qDebug() << "Deleting coupling";

                q_deleteCoupling.bind(1, stopId);
                q_deleteCoupling.bind(2, rsId);
                ret = q_deleteCoupling.execute();
                q_deleteCoupling.reset();

                if(ret != SQLITE_OK)
                {
                    qWarning() << "Error while deleting next uncoupling op. Stop:" << stopId
                               << "Rs:" << rsId << "Op: Uncoupled " << "Ret:" << ret
                               << mDb.error_msg();
                }
            }
            else
            {
                qDebug() << "Keeping couple";
            }
        }
    }
    else
    {
        int row = uncoupled.indexOf(rsId);
        if(row == -1)
            return false;

        q_deleteCoupling.bind(1, m_stopId);
        q_deleteCoupling.bind(2, rsId);
        int ret = q_deleteCoupling.execute();
        q_deleteCoupling.reset();

        if(ret != SQLITE_OK)
        {
            qWarning() << "Error while deleting coupling op. Stop:" << m_stopId
                       << "Rs:" << rsId << "Op: Uncoupled " << "Ret:" << ret
                       << mDb.error_msg();
            return false;
        }

        uncoupled.removeAt(row);
    }

    return true;
}

bool RSCouplingInterface::hasEngineAfterStop(bool *isElectricOnNonElectrifiedLine)
{
    query q_hasEngine(mDb, "SELECT coupling.rs_id,MAX(rs_models.sub_type),MAX(stops.arrival)"
                           " FROM stops"
                           " JOIN coupling ON coupling.stop_id=stops.id"
                           " JOIN rs_list ON rs_list.id=coupling.rs_id"
                           " JOIN rs_models ON rs_models.id=rs_list.model_id"
                           " WHERE stops.job_id=? AND stops.arrival<=? AND rs_models.type=0"
                           " GROUP BY coupling.rs_id"
                           " HAVING coupling.operation=1"
                           " LIMIT 1");
    q_hasEngine.bind(1, m_jobId);
    q_hasEngine.bind(2, arrival);
    if(q_hasEngine.step() != SQLITE_ROW)
        return false; //No engine

    if(isElectricOnNonElectrifiedLine)
    {
        RsEngineSubType subType = RsEngineSubType(q_hasEngine.getRows().get<int>(1));
        *isElectricOnNonElectrifiedLine = (subType == RsEngineSubType::Electric) && (getLineType() != LineType::Electric);
    }
    return true;
}

LineType RSCouplingInterface::getLineType() const
{
    return stopsModel->getLineTypeAfterStop(m_stopId);
}

db_id RSCouplingInterface::getJobId() const
{
    return m_jobId;
}
