#ifndef IGRAPHSCENECOLLECTION_H
#define IGRAPHSCENECOLLECTION_H

#include <QString>

class IGraphScene;

/*!
 * \brief The IGraphSceneCollection class
 *
 * Represents a forward iterable collection of IGraphScene
 */
class IGraphSceneCollection
{
public:

    /*!
     * \brief The SceneItem struct
     */
    struct SceneItem
    {
        IGraphScene *scene = nullptr; //! pointer to scene
        QString name; //! scene title
        QString type; //! scene type name
    };

    IGraphSceneCollection();
    virtual ~IGraphSceneCollection();

    /*!
     * \brief getItemCount
     * \return total item count in this collection
     */
    virtual qint64 getItemCount() = 0;

    /*!
     * \brief startIteration
     * \return true on success
     *
     * Start iteration again. Must be called before first \ref getNextItem()
     * Can also be called to start iterating again from first item.
     */
    virtual bool startIteration() = 0;

    /*!
     * \brief getNextItem
     * \return item struct with scene and some infos
     *
     * Gets next item in collection.
     * The caller is responsible to free \ref SceneItem::scene pointer
     * If iteration got after last item, \ref SceneItem::scene is nullptr
     */
    virtual SceneItem getNextItem() = 0;
};

#endif // IGRAPHSCENECOLLECTION_H
