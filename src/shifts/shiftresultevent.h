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

#ifndef SHIFTRESULTEVENT_H
#define SHIFTRESULTEVENT_H

#include <QEvent>
#include "utils/worker_event_types.h"

#include <QList>
#include "shiftitem.h"

class ShiftResultEvent : public QEvent
{
public:
    static const Type _Type = Type(CustomEvents::ShiftWorkerResult);

    ShiftResultEvent();

    QList<ShiftItem> items;
    int firstRow;
};

#endif // SHIFTRESULTEVENT_H
