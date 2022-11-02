#ifndef BACKGROUNDMANAGER_H
#define BACKGROUNDMANAGER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include <QObject>
#include <QVector>

class IBackgroundChecker;

//TODO: show a progress bar for all task like Qt Creator does
class BackgroundManager : public QObject
{
    Q_OBJECT
public:
    explicit BackgroundManager(QObject *parent = nullptr);
    ~BackgroundManager() override;

    void startAllCheckers();
    void abortAllTasks();
    bool isRunning();

    void addChecker(IBackgroundChecker *mgr);
    void removeChecker(IBackgroundChecker *mgr);

    void clearResults();

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

    void checkerAdded(IBackgroundChecker *mgr);
    void checkerRemoved(IBackgroundChecker *mgr);

private:
    friend class BackgroundResultPanel;
    QVector<IBackgroundChecker *> checkers;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // BACKGROUNDMANAGER_H
