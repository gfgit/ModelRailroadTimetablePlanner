/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef RSCOUPLINGINTERFACE_H
#define RSCOUPLINGINTERFACE_H

#include <QObject>

#include <QList>

#include "sqlite3pp/sqlite3pp.h"
using namespace sqlite3pp;

#include "utils/types.h"

class StopModel;

class RSCouplingInterface : public QObject
{
    Q_OBJECT
public:
    explicit RSCouplingInterface(database &db, QObject *parent = nullptr);

    void loadCouplings(StopModel *model, db_id stopId, db_id jobId, QTime arr);

    bool contains(db_id rsId, RsOp op) const;

    bool coupleRS(db_id rsId, const QString &rsName, bool on, bool checkTractionType);
    bool uncoupleRS(db_id rsId, const QString &rsName, bool on);

    int importRSFromJob(db_id otherStopId);

    bool hasEngineAfterStop(bool *isElectricOnNonElectrifiedLine = nullptr);

    bool isRailwayElectrified() const;

    db_id getJobId() const;

private:
    StopModel *stopsModel;

    QList<db_id> coupled;
    QList<db_id> uncoupled;

    database &mDb;
    command q_deleteCoupling;
    command q_addCoupling;

    db_id m_stopId;
    db_id m_jobId;
    QTime arrival;
};

#endif // RSCOUPLINGINTERFACE_H
