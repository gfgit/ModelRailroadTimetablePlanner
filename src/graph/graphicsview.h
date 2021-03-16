#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>

class HourPane;
class StationLayer;
class BackgroundHelper;
class GraphManager;

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView(GraphManager *mgr, QWidget *parent = nullptr);

    void redrawStationNames();

public slots:
    void updateStationVertOffset();
    void updateHourHorizOffset();

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    bool viewportEvent(QEvent *e) override;

private:
    BackgroundHelper *helper;
    HourPane *hourPane;
    StationLayer *stationLayer;
};

#endif // GRAPHICSVIEW_H
