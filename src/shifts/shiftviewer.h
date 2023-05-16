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

#ifndef SHIFTVIEWER_H
#define SHIFTVIEWER_H

#include <QWidget>

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>

using namespace sqlite3pp;

class QTableView;
class ShiftJobsModel;

class ShiftViewer : public QWidget
{
    Q_OBJECT
public:
    ShiftViewer(QWidget *parent = nullptr);
    virtual ~ShiftViewer();

    void updateName();
    void updateJobsModel();
    void setShift(db_id id);

private slots:
    void showContextMenu(const QPoint &pos);

private:
    db_id shiftId;

    query q_shiftName;

    ShiftJobsModel *model;
    QTableView *view;
};

#endif // SHIFTVIEWER_H
