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
    void onIndexClicked(const QModelIndex &index);
    void onNewJob();
    void onRemove();
    void onRemoveAllJobs();

    void onNewJobSamePath();

private:
    QTableView *view;
    JobListModel *jobsModel;
};

#endif // JOBSVIEWER_H
