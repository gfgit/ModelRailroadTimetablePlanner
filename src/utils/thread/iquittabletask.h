#ifndef IQUITTABLETASK_H
#define IQUITTABLETASK_H

#include <QRunnable>
#include <QAtomicInteger>

class QObject;
class QEvent;

class IQuittableTask : public QRunnable
{
public:
    IQuittableTask(QObject *receiver);

    void stop();
    void cleanup();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    inline bool wasStopped() const { return mQuit.loadRelaxed(); }
#else
    inline bool wasStopped() const { return mQuit.load(); }
#endif

    /* NOTE: use carefully and only when not running
     *
     * This is particulary useful if you want to re-use
     * an already existend task that has already finished running
     *
     * Because when finishing a task would call 'sendEvent(event, true)'
     * which will set mReceiver to nullptr to allow deleting un-tracked tasks
     * (when aborting tasks for example we call 'stop()' and instead of waiting we clear the pointer
     *  holding the task, so to avoid memory leak the task, which knows it has been aborted, deletes himself)
     *
     * So when we re-use the task we must set mReceiver again
     */
    //NOTE: this i useful to re-use an existent task after it finished because mReceiver is set to nullptr in
    inline void setReceiver(QObject *obj) { mReceiver = obj; }

protected:
    void sendEvent(QEvent *e, bool finish);
    inline QAtomicInteger<bool>* getQuit() { return &mQuit; }

private:
    QObject *mReceiver;
    QAtomicInteger<bool> mPointerGuard;
    QAtomicInteger<bool> mQuit;
};

#endif // IQUITTABLETASK_H
