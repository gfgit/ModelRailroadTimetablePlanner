#ifndef LINEGRAPHSCROLLAREA_H
#define LINEGRAPHSCROLLAREA_H

#include <QScrollArea>

class LineGraphViewport;
class StationLabelsHeader;
class HourPanel;
class LineGraphScene;

//TODO: maybe QAbstractScrollArea???
class LineGraphScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit LineGraphScrollArea(QWidget *parent = nullptr);

    void setScene(LineGraphScene *scene);

signals:
    void syncToolbarToScene();

private slots:
    void resizeHeaders();

protected:
    bool viewportEvent(QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    StationLabelsHeader *stationHeader;
    HourPanel *hourPanel;
};

#endif // LINEGRAPHSCROLLAREA_H
