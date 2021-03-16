#include "helpers.h"

#include <sqlite3pp/sqlite3pp.h>

namespace lines {

double getStationsDistanceInMeters(sqlite3pp::database &db, db_id lineId, db_id stA, db_id stB)
{
    sqlite3pp::query q_getStKmInMeters(db, "SELECT pos_meters FROM railways WHERE lineId=? AND stationId=?");
    q_getStKmInMeters.bind(1, lineId);
    q_getStKmInMeters.bind(2, stA);
    if(q_getStKmInMeters.step() != SQLITE_ROW)
        return 0.0;
    const double posA = q_getStKmInMeters.getRows().get<double>(0);
    q_getStKmInMeters.reset();

    q_getStKmInMeters.bind(1, lineId);
    q_getStKmInMeters.bind(2, stB);
    if(q_getStKmInMeters.step() != SQLITE_ROW)
        return 0.0;
    const double posB = q_getStKmInMeters.getRows().get<double>(0);
    q_getStKmInMeters.reset();

    const double distance = qAbs(posA - posB);
    return distance;
}

} // namespace lines


