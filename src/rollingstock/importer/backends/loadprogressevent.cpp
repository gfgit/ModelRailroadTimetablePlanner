#include "loadprogressevent.h"

LoadProgressEvent::LoadProgressEvent(QRunnable *self, int pr, int m) :
    QEvent(_Type),
    task(self),
    progress(pr),
    max(m)
{

}
