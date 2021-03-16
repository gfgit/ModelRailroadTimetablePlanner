#include "graphicsscene.h"

GraphicsScene::GraphicsScene(QObject *parent) : QGraphicsScene(parent)
{

}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    /* This is needed to clear selection in some nasty cases:
     * 1 - select a job in Line1 (JobPathEditor opens...)
     * 2 - change to Line2 (but the selected job is not in Line2)
     * 3 - JobPathEditor is still open and you want to lose it
     * 4 - clicking in empty area won't emit 'selectionChanged()' because
     * the selection was already emty so we need to subclass
     * and manually emit 'selectionCleared()'
*/
    QGraphicsScene::mousePressEvent(e);

    if(selectedItems().isEmpty())
        emit selectionCleared();
}
