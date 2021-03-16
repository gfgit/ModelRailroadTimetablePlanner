#ifndef STATIONLAYER_H
#define STATIONLAYER_H

#include <QWidget>

class GraphManager;

class StationLayer : public QWidget
{
    Q_OBJECT
public:
    StationLayer(GraphManager *mgr, QWidget *parent = nullptr);

public slots:
    void setScroll(int value);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    GraphManager *graphMgr;
    int horizontalScroll;
};

#endif // STATIONLAYER_H
