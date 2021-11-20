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

    /*!
     * \brief listen to focus events
     *
     * If a children widget gets focus events we do not know it
     * So we install an event filter on every children and listen
     *
     * \sa focusInEvent()
     */
    bool eventFilter(QObject *watched, QEvent *ev) override;

signals:
    void requestRedraw();

public slots:
    void resetToolbarToScene();

private slots:
    void onWidgetGraphChanged(int type, db_id objectId);
    void onSceneGraphChanged(int type, db_id objectId);
    void onSceneDestroyed();

protected:
    /*!
     * \brief Activate scene
     *
     * Activate scene, it will receive requests to show items
     *
     * \sa LineGraphScene::activateScene()
     * \sa LineGraphManager::setActiveScene()
     */
    void focusInEvent(QFocusEvent *e) override;

private:
    LineGraphScene *m_scene;

    LineGraphSelectionWidget *selectionWidget;
    QPushButton *redrawBut;
};

#endif // LINEGRAPHTOOLBAR_H
