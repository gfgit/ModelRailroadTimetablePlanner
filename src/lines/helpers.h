#ifndef HELPERS_H
#define HELPERS_H

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

namespace lines {

double getStationsDistanceInMeters(sqlite3pp::database &db, db_id lineId, db_id stA, db_id stB);

} // namespace lines

#endif // HELPERS_H
