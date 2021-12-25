#include "railwaysegmentsplithelper.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QDebug>

RailwaySegmentSplitHelper::RailwaySegmentSplitHelper(database &db) :
    mDb(db)
{

}

void RailwaySegmentSplitHelper::setInfo(const utils::RailwaySegmentInfo &origInfo,
                                        const utils::RailwaySegmentInfo &newInfo)
{
    origSegInfo = origInfo;
    newSegInfo = newInfo;
}

bool RailwaySegmentSplitHelper::split()
{
    transaction t(mDb);

    QString errMsg;
    RailwaySegmentHelper helper(mDb);

    //Update previous segment and set Out Gate
    if(!helper.setSegmentInfo(origSegInfo.segmentId, false,
                               origSegInfo.segmentName, origSegInfo.type,
                               origSegInfo.distanceMeters, origSegInfo.maxSpeedKmH,
                               origSegInfo.from.gateId, origSegInfo.to.gateId,
                               &errMsg))
    {
        qWarning() << "RailwaySegmentSplitHelper: cannot set out gate" << origSegInfo.to.gateId
                   << errMsg;
        return false;
    }

    //Create new segment
    if(!helper.setSegmentInfo(newSegInfo.segmentId, true,
                               newSegInfo.segmentName, newSegInfo.type,
                               newSegInfo.distanceMeters, newSegInfo.maxSpeedKmH,
                               newSegInfo.from.gateId, newSegInfo.to.gateId,
                               &errMsg))
    {
        qWarning() << "RailwaySegmentSplitHelper: cannot create segment" << errMsg;
        return false;
    }

    //Update Railway Lines
    if(!updateLines())
        return false;

    //FIXME: also update/delete jobs

    t.commit();

    //Refresh graphs and station views
    QSet<db_id> stationsToUpdate;
    stationsToUpdate.insert(origSegInfo.from.stationId);
    stationsToUpdate.insert(origSegInfo.to.stationId);
    stationsToUpdate.insert(newSegInfo.to.stationId);

    emit Session->segmentStationsChanged(origSegInfo.segmentId);
    emit Session->stationPlanChanged(stationsToUpdate);

    return true;
}

bool RailwaySegmentSplitHelper::updateLines()
{
    query q_getMaxSegPos(mDb, "SELECT MAX(pos) FROM line_segments WHERE line_id=?");
    command q_setPos(mDb, "UPDATE line_segments SET pos=? WHERE id=?");
    command q_moveSegBy(mDb, "UPDATE line_segments SET pos=pos+? WHERE line_id=? AND pos>=? AND pos<=?");
    command q_newSeg(mDb, "INSERT INTO line_segments(id,line_id,seg_id,direction,pos)"
                          " VALUES(NULL,?,?,?,?)");

    query q(mDb, "SELECT id,line_id,direction,pos FROM line_segments WHERE seg_id=?");
    q.bind(1, origSegInfo.segmentId);
    for(auto line : q)
    {
        db_id lineSegId = line.get<db_id>(0);
        db_id lineId = line.get<db_id>(1);
        bool reversed = line.get<int>(2) != 0;
        int segPos = line.get<int>(3);

        //Get segment max position to avoid triggering UNIQUE constraint
        q_getMaxSegPos.bind(1, lineId);
        q_getMaxSegPos.step();
        const int maxPos = q_getMaxSegPos.getRows().get<int>(0);
        q_getMaxSegPos.reset();

        //Shift to 2 after max position to avoid UNIQUE(line_id,pos) constraint
        //1 after max + 1 new pos wich will be added
        const int shiftPos = maxPos + 2;

        int prevSegPos = segPos;
        int newSegPos = segPos + 1;
        if(reversed)
            qSwap(newSegPos, prevSegPos);

        if(maxPos > segPos)
        {
            //Shift next segments by maxPos
            q_moveSegBy.bind(1, shiftPos);
            q_moveSegBy.bind(2, lineId);
            q_moveSegBy.bind(3, segPos);
            q_moveSegBy.bind(4, maxPos);
            if(q_moveSegBy.execute() != SQLITE_OK)
                return false;
            q_moveSegBy.reset();
        }

        if(prevSegPos != segPos)
        {
            //Set new pos to original segment
            q_setPos.bind(1, prevSegPos);
            q_setPos.bind(2, lineSegId);
            if(q_setPos.execute() != SQLITE_OK)
                return false;
            q_setPos.reset();
        }

        //Create new segment
        q_newSeg.bind(1, lineId);
        q_newSeg.bind(2, newSegInfo.segmentId);
        q_newSeg.bind(3, reversed ? 1 : 0);
        q_newSeg.bind(4, newSegPos);
        if(q_newSeg.execute() != SQLITE_OK)
            return false;
        q_newSeg.reset();

        if(maxPos > segPos)
        {
            //Move segments back but +1 for new segment slot
            q_moveSegBy.bind(1, -shiftPos + 1);
            q_moveSegBy.bind(2, lineId);
            q_moveSegBy.bind(3, shiftPos); //First moved
            q_moveSegBy.bind(4, maxPos + shiftPos); //Last moved
            if(q_moveSegBy.execute() != SQLITE_OK)
                return false;
            q_moveSegBy.reset();
        }
    }

    return true;
}
