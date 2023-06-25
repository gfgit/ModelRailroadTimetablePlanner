/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "shiftgraphscenecollection.h"

#include "shiftgraphscene.h"

ShiftGraphSceneCollection::ShiftGraphSceneCollection(sqlite3pp::database &db) :
    mDb(db),
    curIdx(-1)
{
}

qint64 ShiftGraphSceneCollection::getItemCount()
{
    // We have only 1 shift graph
    return 1;
}

bool ShiftGraphSceneCollection::startIteration()
{
    // There is not iteration for shift graph, fake it.
    curIdx = 0;
    return true;
}

IGraphSceneCollection::SceneItem ShiftGraphSceneCollection::getNextItem()
{
    SceneItem item;
    if (curIdx != 0)
        return item; // Tell caller we ended iteration

    curIdx++;

    // Create new scene without parent so ownership is passed to caller
    ShiftGraphScene *shiftScene = new ShiftGraphScene(mDb);
    shiftScene->loadShifts();

    item.scene = shiftScene;
    item.name  = ShiftGraphScene::tr("Shift Graph");
    item.type  = ShiftGraphScene::tr("shift_graph");
    return item;
}
