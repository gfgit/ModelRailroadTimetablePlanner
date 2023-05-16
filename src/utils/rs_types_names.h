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

#ifndef RS_TYPES_NAMES_H
#define RS_TYPES_NAMES_H

#include <QCoreApplication>
#include "types.h"

static const char* RsTypeNamesTable[] = {
    QT_TRANSLATE_NOOP("RsTypeNames", "Engine"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Freight Wagon"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Coach")
};

static const char* RsSubTypeNamesTable[] = {
    QT_TRANSLATE_NOOP("RsTypeNames", "Not set!"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Electric"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Diesel"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Steam")
};


class RsTypeNames
{
    Q_DECLARE_TR_FUNCTIONS(RsTypeNames)

public:
    static inline QString name(RsType t)
    {
        if(t >= RsType::NTypes)
            return QString();
        return tr(RsTypeNamesTable[int(t)]);
    }

    static inline QString name(RsEngineSubType t)
    {
        if(t >= RsEngineSubType::NTypes)
            return QString();
        return tr(RsSubTypeNamesTable[int(t)]);
    }
};

#endif // RS_TYPES_NAMES_H
