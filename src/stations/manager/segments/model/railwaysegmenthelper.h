#ifndef RAILWAYSEGMENTHELPER_H
#define RAILWAYSEGMENTHELPER_H

#include "utils/types.h"
#include "stations/station_utils.h"

#include <QString>

namespace sqlite3pp {
class database;
} // namespace sqlite3pp

class RailwaySegmentHelper
{
public:
    RailwaySegmentHelper(sqlite3pp::database &db);

    bool getSegmentInfo(utils::RailwaySegmentInfo &info);

    bool getSegmentInfoFromGate(db_id gateId, utils::RailwaySegmentInfo &info);

    bool setSegmentInfo(db_id& segmentId, bool create,
                        const QString &name, utils::RailwaySegmentType type,
                        int distance, int speed,
                        db_id fromGateId, db_id toGateId,
                        QString *outErrMsg);

    bool removeSegment(db_id segmentId, QString *outErrMsg);

    bool findFirstLineOrSegment(db_id &graphObjId, bool &isLine);

private:
    sqlite3pp::database &mDb;
};

#endif // RAILWAYSEGMENTHELPER_H
