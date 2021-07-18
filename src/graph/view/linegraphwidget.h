#ifndef LINEGRAPHWIDGET_H
#define LINEGRAPHWIDGET_H

#include <QWidget>

#include "utils/types.h"

class LineGraphScene;
class LineGraphView;
class LineGraphToolbar;

class LineGraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LineGraphWidget(QWidget *parent = nullptr);

private:
    LineGraphScene *m_scene;

    LineGraphView *view;
    LineGraphToolbar *toolBar;
};

#endif // LINEGRAPHWIDGET_H
