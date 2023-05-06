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

#ifndef IFKFIELD_H
#define IFKFIELD_H

#include <QString>
#include "utils/types.h"

/* IOwnerField
 * Generic interface for RSOwnersSQLDelegate so it can be used by multiple models
 * Used for set a Foreign Key field: user types the name and the model gets the corresponding id
*/

class IFKField
{
public:
    virtual ~IFKField();

    virtual bool getFieldData(int row, int col, db_id &idOut, QString& nameOut) const = 0;
    virtual bool validateData(int row, int col, db_id id, const QString& name) = 0;
    virtual bool setFieldData(int row, int col, db_id id, const QString& name) = 0;
};

#endif // IFKFIELD_H
