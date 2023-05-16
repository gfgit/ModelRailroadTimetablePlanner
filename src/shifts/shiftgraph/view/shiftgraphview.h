/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
