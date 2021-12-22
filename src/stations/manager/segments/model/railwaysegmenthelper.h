#ifndef RAILWAYSEGMENTHELPER_H
#define RAILWAYSEGMENTHELPER_H

#include "utils/types.h"
#include "stations/station_utils.h"

#include <QString>

namespace sqlite3pp {
class database;
} // namespace sqlite3pp

struct RailwaySegmentGateInfo
{
    db_id gateId = 0;
    db_id stationId = 0;
    QString stationName;
    QChar gateLetter;
};

struct RailwaySegmentInfo
{
    db_id segmentId = 0;
    QString segmentName;
    int distanceMeters = 10;
    int maxSpeedKmH = 120;
    utils::RailwaySegmentType type;

    RailwaySegmentGateInfo from;
    RailwaySegmentGateInfo to;
};

class RailwaySegmentHelper
{
public:
    typedef struct RailwaySegmentInfo SegmentInfo;

    RailwaySegmentHelper(sqlite3pp::database &db);

    bool getSegmentInfo(SegmentInfo& info);

    bool getSegmentInfoFromGate(db_id gateId, SegmentInfo& info);

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
