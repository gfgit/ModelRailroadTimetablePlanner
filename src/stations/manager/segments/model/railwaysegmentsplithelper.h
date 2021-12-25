#ifndef RAILWAYSEGMENTSPLITHELPER_H
#define RAILWAYSEGMENTSPLITHELPER_H

#include "railwaysegmenthelper.h"

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
