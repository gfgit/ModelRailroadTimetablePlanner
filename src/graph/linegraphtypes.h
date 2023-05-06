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

#ifndef LINEGRAPHTYPES_H
#define LINEGRAPHTYPES_H

#include <QString>

/*!
 * \brief Enum to describe view content type
 */
enum class LineGraphType
{
    NoGraph = 0, //!< No content displayed
    SingleStation, //!< Show a single station
    RailwaySegment, //!< Show two adjacent stations and the segment in between
    RailwayLine, //!< Show a complete railway line (multiple adjacent segments)
    NTypes
};

namespace utils {

QString getLineGraphTypeName(LineGraphType type);

} // namespace utils

#endif // LINEGRAPHTYPES_H
