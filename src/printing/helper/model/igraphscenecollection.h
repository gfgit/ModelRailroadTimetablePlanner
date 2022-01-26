#ifndef IGRAPHSCENECOLLECTION_H
#define IGRAPHSCENECOLLECTION_H

#include <QString>

class IGraphScene;

class IGraphSceneCollection
{
public:

    struct SceneItem
    {
        IGraphScene *scene = nullptr;
        QString name;
        QString type;
    };

    IGraphSceneCollection();
    virtual ~IGraphSceneCollection();

    virtual qint64 getItemCount() = 0;
    virtual bool startIteration() = 0;
    virtual SceneItem getNextItem() = 0;
    virtual void takeOwnershipOfLastScene() = 0;
};

#endif // IGRAPHSCENECOLLECTION_H
