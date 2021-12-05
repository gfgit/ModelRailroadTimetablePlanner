#ifndef RSCHECKERMANAGER_H
#define RSCHECKERMANAGER_H

#ifdef ENABLE_RS_CHECKER

#include <QObject>

#include <QVector>
#include <QSet>

#include "utils/types.h"

class RsErrWorker;
class RsErrorTreeModel;

class RsCheckerManager : public QObject
{
    Q_OBJECT
public:
    explicit RsCheckerManager(QObject *parent = nullptr);
    ~RsCheckerManager();

    bool event(QEvent *e) override;

    bool startWorker();
    void abortTasks();
    inline bool isRunning() { return m_mainWorker || m_workers.size() > 0; }

    void checkRs(const QSet<db_id> &rsIds);

    RsErrorTreeModel *getErrorsModel() const;

    void clearModel();

signals:
    void progressMax(int max);
    void progress(int val);
    void taskFinished();

public slots:
    void onRSPlanChanged(const QSet<db_id> &rsIds);

private:
    RsErrWorker *m_mainWorker; //Checks all rollingstock
    QVector<RsErrWorker *> m_workers; //Specific check for some RS

    RsErrorTreeModel *errorsModel;
};

#endif // ENABLE_RS_CHECKER

#endif // RSCHECKERMANAGER_H
