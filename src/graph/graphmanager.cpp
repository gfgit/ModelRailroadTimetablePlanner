#include "graphmanager.h"

#include "app/session.h"
#include "viewmanager/viewmanager.h"

#include "utils/model_roles.h"

#include "lines/linestorage.h"

#include "app/scopedebug.h"

#include <QGraphicsSimpleTextItem>

GraphManager::GraphManager(QObject *parent) :
    QObject(parent),
    curLineId(0),
    curJobId(0)
{

    lineStorage = Session->mLineStorage;

    connect(&AppSettings, &MRTPSettings::jobGraphOptionsChanged, this, &GraphManager::updateGraphOptions);
    updateGraphOptions();

    connect(lineStorage, &LineStorage::lineNameChanged, this, &GraphManager::onLineNameChanged);
    connect(lineStorage, &LineStorage::lineStationsModified, this, &GraphManager::onLineModified);
    connect(lineStorage, &LineStorage::lineRemoved, this, &GraphManager::onLineRemoved);
}

GraphManager::~GraphManager()
{

}

void GraphManager::onSelectionChanged()
{
    //TODO: single selection. Ctrl+click allow to select multiple items but only the first one is considerated
//    QGraphicsScene *scene/* = view->scene()*/;
//    if(scene && !scene->selectedItems().isEmpty())
//    {
//        auto sel = scene->selectedItems();
//        QGraphicsItem *item = sel.first();
//        db_id jobId = item->data(JOB_ID_ROLE).toLongLong();

//        if(curJobId == jobId)
//            return;

//        curJobId = jobId;

//        Session->getViewManager()->requestJobEditor(jobId);

//        emit jobSelected(jobId);
//    }
//    else
//    {
//        onSelectionCleared();
//    }
}

void GraphManager::onSelectionCleared()
{
    curJobId = 0;
    Session->getViewManager()->requestClearJob();
    emit jobSelected(0);
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

/* db_id GraphManager::showFirstLine()
 * Tries to select first line in Alphabetical order
 * If it succeds returs its lineId
 * Otherwise returns 0
 */
db_id GraphManager::getFirstLineId()
{
    db_id firstLineId = 0;

    if(Session->m_Db.db())
    {
        query q(Session->m_Db, "SELECT id,MIN(name) FROM lines");
        q.step();
        firstLineId = q.getRows().get<db_id>(0);
    }

    return firstLineId;
}

db_id GraphManager::getCurLineId() const
{
    return curLineId;
}

void GraphManager::updateGraphOptions()
{
    //TODO: maybe get rid of theese variables in MeetingSession and always use AppSettings?
    int hourOffset = AppSettings.getHourOffset();
    Session->hourOffset = hourOffset;

    int horizOffset = AppSettings.getHorizontalOffset();
    Session->horizOffset = horizOffset;

    int vertOffset = AppSettings.getVerticalOffset();
    Session->vertOffset = vertOffset;

    Session->stationOffset = AppSettings.getStationOffset();
    Session->platformOffset = AppSettings.getPlatformOffset();

    Session->jobLineWidth = AppSettings.getJobLineWidth();
}

void GraphManager::onLineNameChanged(db_id lineId)
{
    if(lineId == curLineId)
    {
        //Emit the signal again
        //So MainWindow->lineComboSearch (CustomCompletionLineEdit)
        //updates the text
        emit currentLineChanged(curLineId);
    }
}

void GraphManager::onLineModified(db_id lineId)
{
//    if(lineId == curLineId)
//    {
//        view->redrawStationNames();
//    }
}

void GraphManager::onLineRemoved(db_id lineId)
{
    if(curLineId == lineId)
    {
        //Current line is removed, show another line instead
        lineId = getFirstLineId();
    }
}
