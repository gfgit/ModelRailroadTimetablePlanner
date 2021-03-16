#ifndef SHIFTSCENE_H
#define SHIFTSCENE_H

#include <QGraphicsScene>

#include "utils/types.h"

class ShiftScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit ShiftScene(QObject *parent = nullptr);

    qreal hourOffset;
    qreal horizOffset;
    qreal vertOffset;
    QPen linePen;

signals:
    void jobDoubleClicked(db_id jobId);

protected:
    virtual void drawBackground(QPainter *painter, const QRectF &rect) override;

    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) override;
};

#endif // SHIFTSCENE_H
