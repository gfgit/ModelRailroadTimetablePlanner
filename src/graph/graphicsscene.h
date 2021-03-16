#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QGraphicsScene>

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit GraphicsScene(QObject *parent = nullptr);

signals:
    void selectionCleared();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *e);
};

#endif // GRAPHICSSCENE_H
