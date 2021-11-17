#include "graphmanager.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

GraphManager::GraphManager(QObject *parent) :
    QObject(parent),
    curLineId(0)
{
}

JobSelection GraphManager::getSelectedJob()
{
//    if(!view)
//        return {0, 0}; //NULL
//    QGraphicsScene *scene = view->scene();
//    if(!scene)
//        return {0, 0};

//    auto selection = scene->selectedItems();
//    if(selection.isEmpty())
//        return {0, 0};

//    QGraphicsItem *item = selection.first();
//    db_id jobId = item->data(JOB_ID_ROLE).toLongLong();
//    db_id segmentId = item->data(SEGMENT_ROLE).toLongLong();

//    return {jobId, segmentId};
}

void GraphManager::clearSelection()
{
//    QGraphicsScene *scene = view->scene();
//    if(scene)
//    {
//        scene->clearSelection();
//    }
}

db_id GraphManager::getCurLineId() const
{
    return curLineId;
}
