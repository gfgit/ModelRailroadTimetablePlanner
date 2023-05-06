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

#ifndef MODEL_ROLES_H
#define MODEL_ROLES_H

#include <qnamespace.h>

//Useful constants to set/retrive data from models




#define JOB_ID_ROLE         (Qt::UserRole + 10)

#define RS_MODEL_ID         (Qt::UserRole + 40)
#define RS_TYPE_ROLE        (Qt::UserRole + 41)
#define RS_SUB_TYPE_ROLE    (Qt::UserRole + 42)
#define RS_NUMBER           (Qt::UserRole + 44)
#define RS_IS_ENGINE        (Qt::UserRole + 45)

#define COLOR_ROLE          (Qt::UserRole + 46)

#endif // MODEL_ROLES_H
