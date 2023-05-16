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

#ifndef RSJOBVIEWER_H
#define RSJOBVIEWER_H

#include <QWidget>

#include "utils/types.h"

class QTimeEdit;
class QTableView;
class QLabel;
class RsPlanModel;

class RSJobViewer : public QWidget
{
    Q_OBJECT
public:
    RSJobViewer(QWidget *parent = nullptr);
    virtual ~RSJobViewer();

    void setRS(db_id id);
    db_id m_rsId;

    void updatePlan();
    void updateRsInfo();

public slots:
    void updateInfo();

private slots:
    void showContextMenu(const QPoint &pos);

private:
    QTableView *view;
    RsPlanModel *model;

    QTimeEdit *timeEdit1;
    QTimeEdit *timeEdit2;
    QLabel *infoLabel;
};

#endif // RSJOBVIEWER_H
