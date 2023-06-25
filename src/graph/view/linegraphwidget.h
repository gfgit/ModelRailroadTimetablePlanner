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

#ifndef LINEGRAPHWIDGET_H
#define LINEGRAPHWIDGET_H

#include <QWidget>

#include "utils/types.h"

#include "graph/linegraphtypes.h"

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

    inline LineGraphScene *getScene() const
    {
        return m_scene;
    }

    inline LineGraphView *getView() const
    {
        return view;
    }

    inline LineGraphToolbar *getToolbar() const
    {
        return toolBar;
    }

    bool tryLoadGraph(db_id graphObjId, LineGraphType type);

private:
    LineGraphScene *m_scene;

    LineGraphView *view;
    LineGraphToolbar *toolBar;
};

#endif // LINEGRAPHWIDGET_H
