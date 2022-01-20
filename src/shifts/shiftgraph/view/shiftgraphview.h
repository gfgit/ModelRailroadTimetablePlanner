#ifndef SHIFTGRAPHVIEW_H
#define SHIFTGRAPHVIEW_H

#include "utils/scene/basicgraphview.h"

/*!
 * \brief BasicGraphView subclass to display a ShiftGraphScene
 *
 * A custom view to render ShiftGraphScene contents.
 * Moving the mouse cursor on the contents, tooltips will be shown.
 * Right clicking on a Job shows context menu.
 *
 * \sa ShiftGraphScene
 */
class ShiftGraphView : public BasicGraphView
{
    Q_OBJECT
public:
    explicit ShiftGraphView(QWidget *parent = nullptr);

protected:
    /*!
     * \brief Show Tooltips and context menu
     *
     * Show tooltips on viewport and context menu
     * \sa ShiftGraphScene::getJobAt()
     */
    bool viewportEvent(QEvent *e) override;
};

#endif // SHIFTGRAPHVIEW_H
