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

#ifndef JOBCHANGESHIFTDLG_H
#define JOBCHANGESHIFTDLG_H

#include <QDialog>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class CustomCompletionLineEdit;
class QLabel;

class JobChangeShiftDlg : public QDialog
{
    Q_OBJECT
public:
    JobChangeShiftDlg(sqlite3pp::database &db, QWidget *parent = nullptr);

    void setJob(db_id jobId);
    void setJob(db_id jobId, db_id shiftId, JobCategory jobCat);

public slots:
    virtual void done(int ret) override;

private slots:
    void onJobChanged(db_id jobId, db_id oldJobId);

private:
    db_id mJobId;
    db_id originalShiftId;
    JobCategory mCategory;

    CustomCompletionLineEdit *shiftCombo;
    QLabel *label;

    sqlite3pp::database &mDb;
};

#endif // JOBCHANGESHIFTDLG_H
