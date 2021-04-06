#ifndef RAILWAYSEGMENTHELPER_H
#define RAILWAYSEGMENTHELPER_H

#include "utils/types.h"
#include "stations/station_utils.h"

namespace sqlite3pp {
class database;
} // namespace sqlite3pp

class RailwaySegmentHelper
{
public:
    RailwaySegmentHelper(sqlite3pp::database &db);

    bool getSegmentInfo(db_id segmentId,
                        QString& outName, utils::RailwaySegmentType &outType,
                        int &outDistance, int &outSpeed,
                        db_id& outFromStId, db_id& outFromGateId,
                        db_id& outToStId, db_id& outGateId);

    bool setSegmentInfo(db_id& segmentId, bool create,
                        const QString &name, utils::RailwaySegmentType type,
                        int distance, int speed,
                        db_id fromGateId, db_id toGateId);

    bool removeSegment(db_id segmentId, QString *outErrMsg);

private:
    sqlite3pp::database &mDb;
};

#endif // RAILWAYSEGMENTHELPER_H
