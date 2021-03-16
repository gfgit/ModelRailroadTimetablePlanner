#ifndef BACKGROUNDHELPER_H
#define BACKGROUNDHELPER_H

#include <QObject>

#include <QPen>
#include <QFont>

#include "utils/types.h"

class BackgroundHelper : public QObject
{
    Q_OBJECT
public:
    BackgroundHelper(QObject *parent = nullptr);
    ~BackgroundHelper();

    void setHourLinePen(const QPen &pen);
    void setHourTextPen(const QPen &pen);
    void setHourTextFont(const QFont &font);

    void setHourOffset(qreal value);

    void setVertOffset(qreal value);
    qreal getVertOffset() const;

    void setHourHorizOffset(qreal value);
    qreal getHourHorizOffset() const;

    void drawBackgroundLines(QPainter *painter, const QRectF& rect);
    void drawForegroundHours(QPainter *painter, const QRectF& rect, int scroll);

    void drawForegroundStationLabels(QPainter *painter, const QRectF& rect, int hScroll, db_id lineId);

signals:
    void updateGraph();
    void vertOffsetChanged();
    void horizHorizOffsetChanged();

private:
    qreal hourOffset;
    qreal vertOffset;
    qreal hourHorizOffset;

    QFont hourTextFont;
    QPen hourTextPen;
    QPen hourLinePen;
};

#endif // BACKGROUNDHELPER_H
