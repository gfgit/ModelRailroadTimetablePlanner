#ifndef TASKPROGRESSEVENT_H
#define TASKPROGRESSEVENT_H

#include "utils/worker_event_types.h"

#include <QString>

class IQuittableTask;

class GenericTaskEvent : public QEvent
{
public:
    GenericTaskEvent(QEvent::Type t, IQuittableTask *self);

    IQuittableTask *task = nullptr;
};

class TaskProgressEvent : public GenericTaskEvent
{
public:
    enum
    {
        ProgressError = -1,
        ProgressAbortedByUser = -2,
        ProgressFinished = -3
    };

    static constexpr Type _Type = Type(CustomEvents::TaskProgress);

    TaskProgressEvent(IQuittableTask *self, int pr, int max, const QString& descr = QString());

    int progress = 0;
    int progressMax = 0;
    QString description;
};

#endif // TASKPROGRESSEVENT_H
