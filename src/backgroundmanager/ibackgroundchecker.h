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
