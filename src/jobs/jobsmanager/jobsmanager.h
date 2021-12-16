#ifndef JOBSVIEWER_H
#define JOBSVIEWER_H

#include <QWidget>

class QTableView;
class JobListModel;

class JobsManager : public QWidget
{
    Q_OBJECT
public:
    JobsManager(QWidget *parent = nullptr);

private slots:
    void editJobAtRow(const QModelIndex &idx);

    void onNewJob();
    void onRemove();
    void onNewJobSamePath();
    void onEditJob();
    void onShowJobGraph();

    void onRemoveAllJobs();

    void onSelectionChanged();

private:
    QTableView *view;

    QAction *actRemoveJob;
    QAction *actNewJobSamePath;
    QAction *actEditJob;
    QAction *actShowJobInGraph;

    JobListModel *jobsModel;
};

#endif // JOBSVIEWER_H
