#include "taskprogressevent.h"

GenericTaskEvent::GenericTaskEvent(Type t, IQuittableTask *self) :
    QEvent(t),
    task(self)
{

}

TaskProgressEvent::TaskProgressEvent(IQuittableTask *self, int pr, int max, const QString &descr) :
    GenericTaskEvent(_Type, self),
    progress(pr),
    progressMax(max),
    description(descr)
{

}
