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

#ifndef BACKGROUNDRESULTWIDGET_H
#define BACKGROUNDRESULTWIDGET_H

#ifdef ENABLE_BACKGROUND_MANAGER

#    include <QWidget>

class QTreeView;
class QProgressBar;
class QPushButton;

class IBackgroundChecker;

class BackgroundResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BackgroundResultWidget(IBackgroundChecker *mgr_, QWidget *parent = nullptr);

protected:
    void timerEvent(QTimerEvent *e) override;

private slots:
    void startTask();
    void stopTask();
    void onTaskProgress(int val, int max);
    void taskFinished();
    void showContextMenu(const QPoint &pos);

private:
    friend class BackgroundResultPanel;

    IBackgroundChecker *mgr;
    QTreeView *view;
    QProgressBar *progressBar;
    QPushButton *startBut;
    QPushButton *stopBut;
    int timerId;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // BACKGROUNDRESULTWIDGET_H
