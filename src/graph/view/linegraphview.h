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
