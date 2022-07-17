#ifndef RAILWAYSEGMENTSPLITHELPER_H
#define RAILWAYSEGMENTSPLITHELPER_H

#include "utils/types.h"
#include "stations/station_utils.h"

#include <QString>

namespace sqlite3pp {
class database;
} // namespace sqlite3pp

class RailwaySegmentSplitHelper
{
public:
    RailwaySegmentSplitHelper(sqlite3pp::database &db);

    void setInfo(const utils::RailwaySegmentInfo& origInfo,
                 const utils::RailwaySegmentInfo& newInfo);

    bool split();

private:
    bool updateLines();

private:
    sqlite3pp::database &mDb;

    utils::RailwaySegmentInfo origSegInfo;
    utils::RailwaySegmentInfo newSegInfo;
};

#endif // RAILWAYSEGMENTSPLITHELPER_H
