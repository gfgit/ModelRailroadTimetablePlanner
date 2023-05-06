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
