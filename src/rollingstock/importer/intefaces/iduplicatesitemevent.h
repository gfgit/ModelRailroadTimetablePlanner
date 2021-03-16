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
