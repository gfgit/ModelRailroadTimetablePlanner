#ifndef SHIFTGRAPHSCENECOLLECTION_H
#define SHIFTGRAPHSCENECOLLECTION_H

#include "printing/helper/model/igraphscenecollection.h"

namespace sqlite3pp {
class database;
}

class ShiftGraphSceneCollection : public IGraphSceneCollection
{
public:
    ShiftGraphSceneCollection(sqlite3pp::database &db);

    qint64 getItemCount() override;
    bool startIteration() override;
    SceneItem getNextItem() override;

private:
    sqlite3pp::database &mDb;
    int curIdx;
};

#endif // SHIFTGRAPHSCENECOLLECTION_H
