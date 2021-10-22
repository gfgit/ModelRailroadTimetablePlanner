#ifndef LINEGRAPHTOOLBAR_H
#define LINEGRAPHTOOLBAR_H

#include <QWidget>

#include "utils/types.h"

class LineGraphScene;
class LineGraphSelectionWidget;
class QPushButton;

/*!
 * \brief Toolbar to select and refresh graph
 *
 * \sa LineGraphSelectionWidget
 */
class LineGraphToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit LineGraphToolbar(QWidget *parent = nullptr);
    ~LineGraphToolbar();

    void setScene(LineGraphScene *scene);

signals:
    void requestRedraw();

public slots:
    void resetToolbarToScene();

private slots:
    void onWidgetGraphChanged(int type, db_id objectId);
    void onSceneGraphChanged(int type, db_id objectId);
    void onSceneDestroyed();

private:
    LineGraphScene *m_scene;

    LineGraphSelectionWidget *selectionWidget;
    QPushButton *redrawBut;
};

#endif // LINEGRAPHTOOLBAR_H
