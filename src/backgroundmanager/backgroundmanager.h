/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef BACKGROUNDMANAGER_H
#define BACKGROUNDMANAGER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#    include <QObject>
#    include <QList>

class IBackgroundChecker;

// TODO: show a progress bar for all task like Qt Creator does
class BackgroundManager : public QObject
{
    Q_OBJECT
public:
    explicit BackgroundManager(QObject *parent = nullptr);
    ~BackgroundManager() override;

    void handleSessionLoaded();
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
    QList<IBackgroundChecker *> checkers;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // BACKGROUNDMANAGER_H
