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

#ifndef NEWJOBSAMEPATHDLG_H
#define NEWJOBSAMEPATHDLG_H

#include <QDialog>

#include "utils/types.h"
#include <QTime>

class QLabel;
class QCheckBox;
class QTimeEdit;

class NewJobSamePathDlg : public QDialog
{
    Q_OBJECT
public:
    explicit NewJobSamePathDlg(QWidget *parent = nullptr);

    void setSourceJob(db_id jobId, JobCategory cat, const QTime &start, const QTime &end);

    QTime getNewStartTime() const;
    bool shouldCopyRs() const;
    bool shouldReversePath() const;

private slots:
    void checkTimeIsValid();

private:
    QLabel *label;
    QTimeEdit *startTimeEdit;
    QCheckBox *copyRsCheck;
    QCheckBox *reversePathCheck;

    db_id sourceJobId        = 0;
    JobCategory sourceJobCat = JobCategory::NCategories;
    QTime sourceStart;
    QTime sourceEnd;
};

#endif // NEWJOBSAMEPATHDLG_H
