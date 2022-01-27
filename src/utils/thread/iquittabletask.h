#ifndef IQUITTABLETASK_H
#define IQUITTABLETASK_H

#include <QRunnable>
#include <QAtomicInteger>

class QObject;
class QEvent;

/*!
 * \brief The IQuittableTask class
 *
 * Helper class to get progress report and to be able to stop background tasks
 *
 * You must implement QRunnable::run() method and always send an event when done
 * so the task can be cleaned up properly.
 *
 * \sa sendEvent()
 */
class IQuittableTask : public QRunnable
{
public:
    /*!
     * \brief IQuittableTask
     * \param receiver An object which will receive progress and 'Finished' events
     *
     * \sa sendEvent()
     * \sa setReceiver()
     */
    IQuittableTask(QObject *receiver);

    /*!
     * \brief stop task
     *
     * Send a stop request.
     * If task is busy it might not check it immediately.
     */
    void stop();

    /*!
     * \brief cleanup task
     *
     * The creator of task (on main thread usally) is also responsible for deleting it.
     * But you must wait it finishes before deleting the task otherwise program might crash.
     *
     * If you do not want to wait you can call this function AFTER \ref stop() to tell task
     * to delete itself when it's done.
     * After calling you must clear task pointer because it might have been deleted already.
     */
    void cleanup();

    /*!
     * \brief check for a stop request
     *
     * This function can be called from INSIDE the task to check
     * if it was requested to stop.
     *
     * Do not check too frequently to avoid performance penalty
     */
    inline bool wasStopped() const
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        return mQuit.loadRelaxed();
#else
        return mQuit.load();
#endif
    }

    /*!
     * \brief set receiver object
     * \param obj A receiver object
     *
     * NOTE: this must be called when task in not running (before starting or after finished)
     *
     * This is useful if you want to re-use the task.
     * To do so, set again the receiver before re-starting, because it got cleared when task finished.
     */
    inline void setReceiver(QObject *obj) { mReceiver = obj; }

    /*!
     * \brief lock task to prevent it from being deleted
     *
     * Lock task just before accessing 'this' pointer, i.e. accessing members
     * or calling methods.
     * Then unlock with \ref unlockTask()
     */
    inline void lockTask()
    {
        while (!mPointerGuard.testAndSetOrdered(false, true))
        {
            //Busy wait
        }
    }

    /*!
     * \brief unlock task
     */
    inline void unlockTask()
    {
        mPointerGuard.testAndSetOrdered(true, false);
    }

protected:
    /*!
     * \brief send event to receiver object
     * \param e An event sending status of task
     * \param finish true if task is finished
     *
     * Send an event to receiver (if set), event type must be
     * chosen so that receiver object 'understands' the event and reacts to it.
     * If finish param is true it means no more events will be sent by this task.
     * When finised receiver is set to nullptr or, if already nullptr, task deletes itself.
     */
    void sendEvent(QEvent *e, bool finish);

private:
    QObject *mReceiver;
    QAtomicInteger<bool> mPointerGuard;
    QAtomicInteger<bool> mQuit;
};

#endif // IQUITTABLETASK_H
