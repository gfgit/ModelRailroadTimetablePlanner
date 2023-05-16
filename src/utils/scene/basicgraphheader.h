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

#ifndef BASICGRAPHHEADER_H
#define BASICGRAPHHEADER_H

#include <QWidget>

/*!
 * \brief Helper widget to draw scene header
 *
 * Scene header will be always visible regardless of scrolling
 *
 * \sa IGraphScene
 * \sa BasicGraphView
 */

class IGraphScene;

class BasicGraphHeader : public QWidget
{
    Q_OBJECT
public:
    BasicGraphHeader(Qt::Orientation orient, QWidget *parent = nullptr);

    void setScene(IGraphScene *scene);

public slots:
    void setScroll(int value);
    void setZoom(int value);

protected:
    void paintEvent(QPaintEvent *e);

private:
    IGraphScene *m_scene;

    int m_scroll;
    int mZoom;
    Qt::Orientation m_orientation;
};

#endif // BASICGRAPHHEADER_H
