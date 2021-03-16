#ifndef LOADPROGRESSEVENT_H
#define LOADPROGRESSEVENT_H

#include <QEvent>
#include "utils/worker_event_types.h"

class QRunnable;

class LoadProgressEvent : public QEvent
{
public:
    enum
    {
        ProgressError = -1,
        ProgressAbortedByUser = -2,
        ProgressMaxFinished = -3
    };

    static constexpr Type _Type = Type(CustomEvents::RsImportLoadProgress);

    LoadProgressEvent(QRunnable *self, int pr, int m);

public:
    QRunnable *task;
    int progress;
    int max;
};

#endif // LOADPROGRESSEVENT_H
