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

#ifndef EDITLINEDLG_H
#define EDITLINEDLG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class QTableView;
class QLineEdit;
class KmSpinBox;

class LineSegmentsModel;

class EditLineDlg : public QDialog
{
    Q_OBJECT
public:
    EditLineDlg(sqlite3pp::database &db, QWidget *parent = nullptr);

    virtual void done(int res) override;

    void setLineId(db_id lineId);

private slots:
    void onModelError(const QString& msg);
    void addStation();
    void removeAfterCurrentPos();

private:
    sqlite3pp::database& mDb;
    LineSegmentsModel *model;

    QTableView *view;
    QLineEdit *lineNameEdit;
    KmSpinBox *lineStartKmSpin;
};

#endif // EDITLINEDLG_H
