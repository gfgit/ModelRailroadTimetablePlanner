#ifndef LINEGRAPHVIEW_H
#define LINEGRAPHVIEW_H

#include "utils/scene/basicgraphview.h"

/*!
 * \brief BasicGraphView subclass to display a LineGraphScene
 *
 * A custom view to render LineGraphScene contents.
 * Moving the mouse cursor on the contents, tooltips will be shown.
 * Double clicking on a Job selects it.
 *
 * \sa LineGraphScene
 * \sa LineGraphWidget
 * \sa BackgroundHelper
 */
class LineGraphView : public BasicGraphView
{
    Q_OBJECT
public:
    explicit LineGraphView(QWidget *parent = nullptr);

signals:
    /*!
     * \brief Sync toolbar on click
     *
     * \sa syncToolbarToScene()
     */
    void syncToolbarToScene();

protected:
    /*!
     * \brief Show Tooltips
     *
     * Show tooltips on viewport
     * \sa LineGraphScene::getJobAt()
     */
    bool viewportEvent(QEvent *e) override;

    /*!
     * \brief Sync toolbar on click
     *
     * \sa syncToolbarToScene()
     */
    void mousePressEvent(QMouseEvent *e) override;

    /*!
     * \brief Select jobs
     *
     * Select jobs by double clicking on them
     * \sa LineGraphScene::getJobAt()
     */
    void mouseDoubleClickEvent(QMouseEvent *e) override;
};

#endif // LINEGRAPHVIEW_H
