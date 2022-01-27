#include "iquittabletask.h"

#include <QCoreApplication>

IQuittableTask::IQuittableTask(QObject *receiver) :
    QRunnable(),
    mReceiver(receiver),
    mPointerGuard(false),
    mQuit(false)
{
    //NOTE: we disable Qt auto deletion flafg and handle it ourselves
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
    lockTask();

    if(mReceiver)
    {
        //This means task has not finished yet.
        //Clear receiver to tell task to delete itself when done inside sendEvent()
        mReceiver = nullptr;
        unlockTask();
    }
    else
    {
        //The task was already finished, delete it
        delete this;
    }
}

void IQuittableTask::sendEvent(QEvent *e, bool finish)
{
    lockTask();

    if(mReceiver)
    {
        //Post the event to receiver object
        qApp->postEvent(mReceiver, e);
        if(finish)
            mReceiver = nullptr; //Clear receiver to tell cleanup() to delete task
        unlockTask();
    }
    else
    {
        //Delete event because it cannot be posted
        delete e;
        if(finish)
            delete this; //cleanup() was called, we are responsible for deleting ourselves
        else
            unlockTask();
    }
}
