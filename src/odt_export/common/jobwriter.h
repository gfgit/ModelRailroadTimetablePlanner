#ifndef JOBWRITER_H
#define JOBWRITER_H

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class QXmlStreamWriter;

class JobWriter
{
public:
    JobWriter(database& db);

    static void writeJobAutomaticStyles(QXmlStreamWriter& xml);
    static void writeJobStyles(QXmlStreamWriter &xml);

    void writeJob(QXmlStreamWriter &xml, db_id jobId, JobCategory jobCat);

private:
    database &mDb;

    query q_getJobStops;
    query q_getFirstStop;
    query q_getLastStop;
    query q_initialJobAxes;
    query q_selectPassings;
    query q_getStopCouplings;
};

#endif // JOBWRITER_H
