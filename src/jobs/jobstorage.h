#ifndef JOBSMODEL_H
#define JOBSMODEL_H

#include <QObject>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

//TODO: at loading check if 'stops.rw_node' AND 'stops.other_rw_node' ARE correct
//      otherwise try to fix them and if not possible tell the user, SEE ALSO: SHOW_WRONG_OTHER_RW_NODE.sql, SHOW_WRONG_RW_NODE.sql, FIX_RW_NODE.sql, FIX_OTHER_RW_NODE_PARTIALLY.sql
class JobStoragePrivate;
class JobStorage : public QObject
{
    Q_OBJECT
public:

    JobStorage(sqlite3pp::database& db, QObject *parent = nullptr);
    ~JobStorage();

    void fixJobs();
    void clear();

    bool addJob(db_id *outJobId = nullptr);
    bool removeJob(db_id jobId);
    void removeAllJobs();

    void drawJobs();

    void updateJobPath(db_id jobId);

    void updateFirstLast(db_id jobId);

    bool selectSegment(db_id jobId, db_id segId, bool select, bool ensureVisible);

private:
    friend class LineStoragePrivate;
    void drawJobs(db_id lineId);
    void loadLine(db_id lineId);
    void unloadLine(db_id lineId);

signals:
    void jobAdded(db_id jobId);
    void aboutToRemoveJob(db_id jobId);
    void jobRemoved(db_id jobId);

private slots:
    void updateJobColors();
    void updateJob(db_id newId, db_id oldId);

private:
    JobStoragePrivate *impl;
    sqlite3pp::database& mDb;
};

#endif // JOBSMODEL_H
