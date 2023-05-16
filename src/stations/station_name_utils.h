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

#ifndef STATION_NAME_UTILS_H
#define STATION_NAME_UTILS_H

#include <QCoreApplication>

#include "station_utils.h"

namespace utils {

static const char* StationTypeNamesTable[] = {
    QT_TRANSLATE_NOOP("StationUtils", "Normal"),
    QT_TRANSLATE_NOOP("StationUtils", "Simple Stop"),
    QT_TRANSLATE_NOOP("StationUtils", "Junction")
};

class StationUtils
{
    Q_DECLARE_TR_FUNCTIONS(StationUtils)

public:
    static inline QString name(StationType t)
    {
        if(t >= StationType::NTypes)
            return QString();
        return tr(StationTypeNamesTable[int(t)]);
    }

    static inline QString name(Side s)
    {
        return s == Side::East ? tr("East") : tr("West");
    }
};

} // namespace utils

#endif // STATION_NAME_UTILS_H
