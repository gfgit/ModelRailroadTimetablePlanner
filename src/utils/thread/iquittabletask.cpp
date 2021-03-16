#include "iquittabletask.h"

#include <QCoreApplication>

IQuittableTask::IQuittableTask(QObject *receiver) :
    QRunnable(),
    mReceiver(receiver),
    mPointerGuard(false),
    mQuit(false)
{
    setAutoDelete(false);
}

void IQuittableTask::stop()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    mQuit.storeRelaxed(true);
#else
    mQuit.store(true);
#endif
}

void IQuittableTask::cleanup()
{
    while (!mPointerGuard.testAndSetOrdered(false, true))
    {
        //Busy wait
    }

    if(mReceiver)
    {
        mReceiver = nullptr;
        mPointerGuard.testAndSetOrdered(true, false);
    }
    else
    {
        delete this;
    }
}

void IQuittableTask::sendEvent(QEvent *e, bool finish)
{
    while (!mPointerGuard.testAndSetOrdered(false, true))
    {
        //Busy wait
    }

    if(mReceiver)
    {
        qApp->postEvent(mReceiver, e);
        if(finish)
            mReceiver = nullptr;
        mPointerGuard.testAndSetOrdered(true, false);
    }
    else
    {
        delete e;
        if(finish)
            delete this;
        else
            mPointerGuard.testAndSetOrdered(true, false);
    }
}
