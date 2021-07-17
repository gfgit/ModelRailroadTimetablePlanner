#ifndef LINEGRAPHWIDGET_H
#define LINEGRAPHWIDGET_H

#include <QWidget>

class LineGraphScene;
class LineGraphViewport;
class LineGraphScrollArea;

class LineGraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LineGraphWidget(QWidget *parent = nullptr);

private:
    LineGraphScene *m_scene;

    LineGraphScrollArea *scrollArea;
    LineGraphViewport *viewport;
};

#endif // LINEGRAPHWIDGET_H
