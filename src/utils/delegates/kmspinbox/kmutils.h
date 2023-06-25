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

#ifndef KMUTILS_H
#define KMUTILS_H

#include <QString>

namespace utils {

/* Convinence functions:
 * Km in railways are expressed with fixed decimal point
 * Decimal separator: '+'
 * Deciaml digits: 3 (meters)
 * Format: X+XXX
 *
 * Example: 15.75 km -> KM 15+750
 */
QString kmNumToText(int kmInMeters);
int kmNumFromTextInMeters(const QString &str);

} // namespace utils

#endif // KMUTILS_H
