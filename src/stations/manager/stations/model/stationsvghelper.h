#ifndef STATIONSVGHELPER_H
#define STATIONSVGHELPER_H

#include "utils/types.h"

class QIODevice;

namespace sqlite3pp {
class database;
}

//FIXME: compress SVG on saving and uncompress on loading?
class StationSVGHelper
{
public:

    static bool addImage(sqlite3pp::database &db, db_id stationId, QIODevice *source);
    static bool removeImage(sqlite3pp::database &db, db_id stationId);

    static bool saveImage(sqlite3pp::database &db, db_id stationId, QIODevice *dest);
    static QIODevice* loadImage(sqlite3pp::database &db, db_id stationId);
};

#endif // STATIONSVGHELPER_H
