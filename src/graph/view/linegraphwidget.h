#ifndef LINEGRAPHWIDGET_H
#define LINEGRAPHWIDGET_H

#include <QWidget>

#include "utils/types.h"

class LineGraphScene;
class LineGraphViewport;
class LineGraphToolbar;
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
    LineGraphToolbar *toolBar;
};

#endif // LINEGRAPHWIDGET_H
