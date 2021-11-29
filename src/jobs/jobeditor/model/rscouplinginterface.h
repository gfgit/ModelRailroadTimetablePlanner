#ifndef RSCOUPLINGINTERFACE_H
#define RSCOUPLINGINTERFACE_H

#include <QObject>

#include <QVector>

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

    bool hasEngineAfterStop(bool *isElectricOnNonElectrifiedLine = nullptr);

    bool isRailwayElectrified() const;

    db_id getJobId() const;

private:
    StopModel *stopsModel;

    QVector<db_id> coupled;
    QVector<db_id> uncoupled;

    database &mDb;
    command q_deleteCoupling;
    command q_addCoupling;

    db_id m_stopId;
    db_id m_jobId;
    QTime arrival;
};

#endif // RSCOUPLINGINTERFACE_H
