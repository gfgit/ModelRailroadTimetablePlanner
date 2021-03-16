#ifndef STATIONWRITER_H
#define STATIONWRITER_H

#include <QVector>
#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class QXmlStreamWriter;

class StationWriter
{
public:
    StationWriter(database& db);

    static void writeStationAutomaticStyles(QXmlStreamWriter& xml);

    void writeStation(QXmlStreamWriter &xml, db_id stationId, QString *stNameOut = nullptr);

private:
    typedef struct Stop
    {
        db_id jobId;

        QString prevSt;
        QString nextSt;

        QString description;

        QTime arrival;
        QTime departure;

        int platform;
        JobCategory jobCat;
    } Stop;

    void insertStop(QXmlStreamWriter &xml, const Stop& stop, bool first, bool transit);

private:
    database &mDb;

    query q_getJobsByStation;
    query q_selectPassings;
    query q_getStopCouplings;
};

#endif // STATIONWRITER_H
