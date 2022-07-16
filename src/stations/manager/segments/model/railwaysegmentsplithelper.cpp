#include "railwaysegmentsplithelper.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QCoreApplication>

//Error messages TODO
//class RailwaySegmentSplitHelperStrings
//{
//    Q_DECLARE_TR_FUNCTIONS(RailwaySegmentSplitHelperStrings)
//};

RailwaySegmentSplitHelper::RailwaySegmentSplitHelper(sqlite3pp::database &db) :
    mDb(db)
{

}

void RailwaySegmentSplitHelper::setInfo(const utils::RailwaySegmentInfo &origSeg, const utils::RailwaySegmentInfo &newSeg)
{
    //FIXME: implement
}

bool RailwaySegmentSplitHelper::split()
{
    //FIXME: implement
    return false;
}
