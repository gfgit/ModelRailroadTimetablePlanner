#include "shiftgraphscenecollection.h"

#include "shiftgraphscene.h"

ShiftGraphSceneCollection::ShiftGraphSceneCollection(sqlite3pp::database &db) :
    mDb(db),
    curIdx(-1)
{

}

qint64 ShiftGraphSceneCollection::getItemCount()
{
    //We have only 1 shift graph
    return 1;
}

bool ShiftGraphSceneCollection::startIteration()
{
    //There is not iteration for shift graph, fake it.
    curIdx = 0;
    return true;
}

IGraphSceneCollection::SceneItem ShiftGraphSceneCollection::getNextItem()
{
    SceneItem item;
    if(curIdx != 0)
        return item; //Tell caller we ended iteration

    curIdx++;

    //Create new scene without parent so ownership is passed to caller
    ShiftGraphScene *shiftScene = new ShiftGraphScene(mDb);
    shiftScene->loadShifts();

    item.scene = shiftScene;
    item.name = ShiftGraphScene::tr("Shift Graph");
    item.type = ShiftGraphScene::tr("shift_graph");
    return item;
}
