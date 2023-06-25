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

#ifndef TASKPROGRESSEVENT_H
#define TASKPROGRESSEVENT_H

#include "utils/worker_event_types.h"

#include <QString>

class IQuittableTask;

class GenericTaskEvent : public QEvent
{
public:
    GenericTaskEvent(QEvent::Type type_, IQuittableTask *self);

    IQuittableTask *task = nullptr;
};

class TaskProgressEvent : public GenericTaskEvent
{
public:
    enum
    {
        ProgressError         = -1,
        ProgressAbortedByUser = -2,
        ProgressFinished      = -3
    };

    static constexpr Type _Type = Type(CustomEvents::TaskProgress);

    TaskProgressEvent(IQuittableTask *self, int pr, int max, const QString &descr = QString());

    int progress    = 0;
    int progressMax = 0;
    QString description;
};

#endif // TASKPROGRESSEVENT_H
