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

#ifndef IBACKGROUNDCHECKER_H
#define IBACKGROUNDCHECKER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include <QObject>
#include <QVector>

class QAbstractItemModel;
class QModelIndex;
class QWidget;
class QPoint;

class IQuittableTask;

namespace sqlite3pp {
class database;
}

class IBackgroundChecker : public QObject
{
    Q_OBJECT
public:
    IBackgroundChecker(sqlite3pp::database &db, QObject *parent = nullptr);

    bool event(QEvent *e) override;

    bool startWorker();
    void abortTasks();

    inline bool isRunning() const { return m_mainWorker || m_workers.size() > 0; }

    inline QAbstractItemModel *getModel() const { return errorsModel; };

    virtual QString getName() const = 0;
    virtual void clearModel() = 0;
    virtual void showContextMenu(QWidget *panel, const QPoint& globalPos, const QModelIndex& idx) const = 0;

    virtual void sessionLoadedHandler();

signals:
    void progress(int val, int max);
    void taskFinished();

protected:
    void addSubTask(IQuittableTask *task);

    virtual IQuittableTask *createMainWorker() = 0;
    virtual void setErrors(QEvent *e, bool merge) = 0;

protected:
    sqlite3pp::database &mDb;
    QAbstractItemModel *errorsModel = nullptr;
    int eventType = 0;

private:
    IQuittableTask *m_mainWorker = nullptr;
    QVector<IQuittableTask *> m_workers;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // IBACKGROUNDCHECKER_H
