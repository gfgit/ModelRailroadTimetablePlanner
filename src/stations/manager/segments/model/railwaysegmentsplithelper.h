#ifndef RAILWAYSEGMENTSPLITHELPER_H
#define RAILWAYSEGMENTSPLITHELPER_H

#include "railwaysegmenthelper.h"

class RailwaySegmentSplitHelper
{
public:
    RailwaySegmentSplitHelper(sqlite3pp::database &db, db_id segmentId);

    void setInfo(const RailwaySegmentHelper::SegmentInfo& info,
                 const db_id newOutGate);

    bool split();

private:
    bool updateLines();

private:
    sqlite3pp::database &mDb;

    db_id originalSegmentId;
    db_id m_newOutGate;
    RailwaySegmentHelper::SegmentInfo segInfo;

};

#endif // RAILWAYSEGMENTSPLITHELPER_H
