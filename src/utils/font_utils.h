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

#ifndef FONT_UTILS_H
#define FONT_UTILS_H

#include <QPainter>

/*!
 * \brief Set font point size
 * \param font
 * \param val the value of font size to set
 * \param p the QPainter which will draw
 *
 * This function is needed because each QPaintDevice has different resolution (DPI)
 * The default value is 72 dots per inch (DPI)
 * But for example QPdfWriter default is 1200,
 * QPrinter default is 300 an QWidget depends on the screen,
 * so it depends on Operating System display settings.
 *
 * To avoid differences between screen contents and printed output we
 * rescale font sizes as it would be if DPI was 72
 */
inline void setFontPointSizeDPI(QFont &font, const qreal val, QPainter *p)
{
    const qreal pointSize = val * 72.0 / qreal(p->device()->logicalDpiY());
    font.setPointSizeF(pointSize);
}

#endif // FONT_UTILS_H
