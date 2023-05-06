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

#ifndef RS_UTILS_H
#define RS_UTILS_H

#include <QString>

#include "types.h"

namespace rs_utils
{
//Format RS number according to its type
QString formatNum(RsType type, int number);

QString formatName(const QString& model, int number, const QString& suffix, RsType type);
QString formatNameRef(const char *model, int modelSize, int number, const char *suffix, int suffixSize, RsType type);

}

#endif // RS_UTILS_H
