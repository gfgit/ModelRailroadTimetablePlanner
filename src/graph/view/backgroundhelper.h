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

#ifndef BACKGROUNDHELPER_H
#define BACKGROUNDHELPER_H

#include <QRectF>

class QPainter;

class LineGraphScene;

/*!
 * \brief Helper class to render LineGraphScene contents
 *
 * Contains static helper functions to draw each part of the view
 *
 * \sa LineGraphScene
 */
class BackgroundHelper
{
public:
    static void drawHourPanel(QPainter *painter, const QRectF& rect);

    static void drawBackgroundHourLines(QPainter *painter, const QRectF& rect);

    static void drawStationHeader(QPainter *painter, LineGraphScene *scene, const QRectF& rect);

    static void drawStations(QPainter *painter, LineGraphScene *scene, const QRectF& rect);

    static void drawJobStops(QPainter *painter, LineGraphScene *scene, const QRectF& rect, bool drawSelection);

    static void drawJobSegments(QPainter *painter, LineGraphScene *scene, const QRectF &rect, bool drawSelection);

public:
    static constexpr double SelectedJobWidthFactor = 3.0;
    static constexpr int SelectedJobAlphaFactor = 127;
};

#endif // BACKGROUNDHELPER_H
