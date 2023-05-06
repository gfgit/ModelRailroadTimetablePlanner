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

#ifndef IDUPLICATESITEMEVENT_H
#define IDUPLICATESITEMEVENT_H

#include <QEvent>
#include "utils/worker_event_types.h"

class QRunnable;

class IDuplicatesItemEvent : public QEvent
{
public:
    static constexpr int MinimumMSecsBeforeFirstEvent = 4000;

    enum
    {
        ProgressError = -1,
        ProgressAbortedByUser = -2,
        ProgressMaxFinished = -3
    };

    static constexpr Type _Type = Type(CustomEvents::RsImportCheckDuplicates);

    IDuplicatesItemEvent(QRunnable *self, int pr, int m, int count, int st);

public:
    QRunnable *task;
    int progress;
    int max;
    int ducplicatesCount;
    int state;
};

#endif // IDUPLICATESITEMEVENT_H
