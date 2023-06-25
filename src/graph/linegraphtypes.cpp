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

#include "linegraphtypes.h"

#include <QCoreApplication>

class LineGraphTypeNames
{
    Q_DECLARE_TR_FUNCTIONS(LineGraphTypeNames)
public:
    static const char *texts[];
};

const char *LineGraphTypeNames::texts[] = {QT_TRANSLATE_NOOP("LineGraphTypeNames", "No Graph"),
                                           QT_TRANSLATE_NOOP("LineGraphTypeNames", "Station"),
                                           QT_TRANSLATE_NOOP("LineGraphTypeNames", "Segment"),
                                           QT_TRANSLATE_NOOP("LineGraphTypeNames", "Line")};

QString utils::getLineGraphTypeName(LineGraphType type)
{
    if (type >= LineGraphType::NTypes)
        return QString();
    return LineGraphTypeNames::tr(LineGraphTypeNames::texts[int(type)]);
}
