#include "iduplicatesitemevent.h"

IDuplicatesItemEvent::IDuplicatesItemEvent(QRunnable *self, int pr, int m, int count, int st) :
    QEvent(_Type),
    task(self),
    progress(pr),
    max(m),
    ducplicatesCount(count),
    state(st)
{

}
