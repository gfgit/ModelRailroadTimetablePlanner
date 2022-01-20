#ifndef SHIFTGRAPHVIEW_H
#define SHIFTGRAPHVIEW_H

#include "utils/scene/basicgraphview.h"

class ShiftGraphView : public BasicGraphView
{
    Q_OBJECT
public:
    explicit ShiftGraphView(QWidget *parent = nullptr);

protected:
    /*!
     * \brief Show Tooltips
     *
     * Show tooltips on viewport
     * \sa LineGraphScene::getJobAt()
     */
    bool viewportEvent(QEvent *e) override;
};

#endif // SHIFTGRAPHVIEW_H
