#include "taskprogressevent.h"

GenericTaskEvent::GenericTaskEvent(Type type_, IQuittableTask *self) :
    QEvent(type_),
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
