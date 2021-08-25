#ifndef LINEGRAPHWIDGET_H
#define LINEGRAPHWIDGET_H

#include <QWidget>

#include "utils/types.h"

class LineGraphScene;
class LineGraphView;
class LineGraphToolbar;

/*!
 * \brief An all-in-one widget as a railway line view
 *
 * This widget encapsulate a toolbar to select which
 * contents user wants to see (stations, segments or railway lines)
 * and a view which renders the chosen contents
 *
 * \sa LineGraphToolbar
 * \sa LineGraphView
 */
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
