#ifndef LINEGRAPHWIDGET_H
#define LINEGRAPHWIDGET_H

#include <QWidget>

class LineGraphScene;
class LineGraphViewport;
class StationLabelsHeader;
class HourPanel;
class QScrollArea;

class LineGraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LineGraphWidget(QWidget *parent = nullptr);

private slots:
    void resizeHeaders();

private:
    LineGraphScene *m_scene;

    QScrollArea *scrollArea;
    LineGraphViewport *viewport;
    StationLabelsHeader *stationHeader;
    HourPanel *hourPanel;
};

#endif // LINEGRAPHWIDGET_H
