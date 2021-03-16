#ifndef BACKGROUNDMANAGER_H
#define BACKGROUNDMANAGER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include <QObject>

#ifdef ENABLE_RS_CHECKER
class RsCheckerManager;
#endif

//TODO: show a progress bar for all task like Qt Creator does
class BackgroundManager : public QObject
{
    Q_OBJECT
public:
    explicit BackgroundManager(QObject *parent = nullptr);
    ~BackgroundManager() override;

    void abortAllTasks();
    bool isRunning();

#ifdef ENABLE_RS_CHECKER
    inline RsCheckerManager *getRsChecker() const { return rsChecker; };
#endif // ENABLE_RS_CHECKER

signals:
    /* abortTrivialTasks() signal
     *
     * Stop tasks that are less important like SearchTask
     * but don't stop long running tasks like RsErrWorker
     * This function is called when closing current session
     * (NOTE: opening/creating new session closes the current one)
     * If user changes his mind and still keeps this session
     * then it would have to restart background task again
     *
     * So stop all trivial tasks and wait the user to confirm his choice
     * to close this session before stopping all other tasks
     */
    void abortTrivialTasks();

private:
#ifdef ENABLE_RS_CHECKER
    RsCheckerManager *rsChecker;
#endif
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // BACKGROUNDMANAGER_H
