#ifndef JOBSTOPSIMPORTER_H
#define JOBSTOPSIMPORTER_H

#include <QString>
#include <QTime>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class JobStopsImporter
{
public:
    struct Stop
    {
        QString stationName;
        QTime arrival;
        QTime departure;
        QString platformName;
    };

    JobStopsImporter(sqlite3pp::database &db);

    bool createJob(db_id jobId, JobCategory category, const QVector<Stop>& stops);

private:
    sqlite3pp::database &mDb;
};

#endif // JOBSTOPSIMPORTER_H
