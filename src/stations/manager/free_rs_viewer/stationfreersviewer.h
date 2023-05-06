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

#ifndef STATIONFREERSVIEWER_H
#define STATIONFREERSVIEWER_H

#include <QWidget>

#include "utils/types.h"

class StationFreeRSModel;
class QTimeEdit;
class QTableView;
class QPushButton;

class StationFreeRSViewer : public QWidget
{
    Q_OBJECT
public:
    explicit StationFreeRSViewer(QWidget *parent = nullptr);

    void setStation(db_id stId);
    void updateTitle();
    void updateData();

public slots:
    void goToNext();
    void goToPrev();

private slots:
    void onTimeEditingFinished();

    void showContextMenu(const QPoint &pos);

    void sectionClicked(int col);

private:
    StationFreeRSModel *model;
    QTableView *view;

    QTimeEdit *timeEdit;
    QPushButton *refreshBut;
    QPushButton *nextOpBut;
    QPushButton *prevOpBut;
};

#endif // STATIONFREERSVIEWER_H
