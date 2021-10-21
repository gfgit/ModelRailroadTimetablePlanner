#ifndef STATIONSVGHELPER_H
#define STATIONSVGHELPER_H

#include "utils/types.h"

class QIODevice;

namespace sqlite3pp {
class database;
}

namespace ssplib {
class StationPlan;
}

//FIXME: compress SVG on saving and uncompress on loading?
class StationSVGHelper
{
public:

    static bool addImage(sqlite3pp::database &db, db_id stationId, QIODevice *source);
    static bool removeImage(sqlite3pp::database &db, db_id stationId);

    static bool saveImage(sqlite3pp::database &db, db_id stationId, QIODevice *dest);
    static QIODevice* loadImage(sqlite3pp::database &db, db_id stationId);

    static bool loadStationFromDB(sqlite3pp::database &db, db_id stationId, ssplib::StationPlan *plan);
};

#endif // STATIONSVGHELPER_H
