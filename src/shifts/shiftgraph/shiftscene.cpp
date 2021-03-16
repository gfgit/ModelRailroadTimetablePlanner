#include "shiftscene.h"

#include <QPainter>

#include <QtMath>

#include <QGraphicsSceneMouseEvent>

#include <QGraphicsItem>
#include "utils/model_roles.h"

ShiftScene::ShiftScene(QObject *parent) :
    QGraphicsScene(parent)
{
    linePen.setWidth(4);
    linePen.setColor(Qt::gray);
}

void ShiftScene::drawBackground(QPainter *painter, const QRectF& rect)
{
    const qreal x1 = rect.left();
    const qreal x2 = rect.right();
    const qreal t = qMax(rect.top(), vertOffset);
    const qreal b = rect.bottom();

    int first = qFloor((x1 - horizOffset) / hourOffset);
    qreal firstX = first * hourOffset + horizOffset;

    if(first < 0.0)
    {
        first = 0.0;
        firstX = horizOffset;
    }

    int last = qCeil((x2 - horizOffset) / hourOffset);

    if(first > last)
        return;

    if(rect.top() < vertOffset * 3)
    {
        painter->setPen(Qt::blue);
        painter->setFont(QFont("ARIAL", 10, QFont::Bold));

        const QString fmt = QStringLiteral("%1:00");
        double huorX = firstX;
        for(int i = first; i < last; i++)
        {
            painter->drawText(QPointF(huorX, 15), fmt.arg(i));
            huorX += hourOffset;
        }
    }

    if(t < b)
    {
        size_t n = size_t(last - first + 1);
        QLineF *arr = new QLineF[n];
        double lineX = firstX;
        for(std::size_t i = 0; i < n; i++)
        {
            arr[i] = QLineF(lineX, t, lineX, b);
            lineX += hourOffset;
        }

        painter->setPen(linePen);
        painter->drawLines(arr, int(n));
        delete [] arr;
    }
}

void ShiftScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e)
{
    if(e->button() == Qt::MouseButton::LeftButton)
    {
        QList<QGraphicsItem *> list = items(e->scenePos());
        for(QGraphicsItem *i : list)
        {
            QVariant v = i->data(JOB_ID_ROLE);
            bool ok = false;
            db_id jobId = v.toLongLong(&ok);
            if(ok && jobId != 0)
            {
                emit jobDoubleClicked(jobId);

                e->setAccepted(true);
                return;
            }
        }
    }

    QGraphicsScene::mouseDoubleClickEvent(e);
}
