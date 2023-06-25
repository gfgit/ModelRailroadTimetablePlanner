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

#ifndef JOBPATHEDITOR_H
#define JOBPATHEDITOR_H

#include <QDialog>

#include "utils/types.h"

class StopDelegate;
class CustomCompletionLineEdit;

class StopModel;
class NextPrevRSJobsModel;

namespace Ui {
class JobPathEditor;
}

/*!
 * \brief The JobPathEditor class
 *
 * Widget to edit jobs info and travel path
 *
 * \sa StopModel
 * \sa StopDelegate
 */
class JobPathEditor : public QDialog
{
    Q_OBJECT

public:
    explicit JobPathEditor(QWidget *parent = nullptr);
    ~JobPathEditor() override;

    bool setJob(db_id jobId);
    bool createNewJob(db_id *out = nullptr);

    bool clearJob();

    bool saveChanges();

    void discardChanges();

    db_id currentJobId() const;

    bool isEdited() const;

    bool maybeSave();

    void closeStopEditor();
    void setReadOnly(bool readOnly);

    bool getCanSetJob() const;

    void selectStop(db_id stopId);

public slots:
    void done(int) override;

    void onShiftError();
    void onSaveSheet();

    void updateSpinColor();

protected:
    virtual void timerEvent(QTimerEvent *e) override;
    virtual void closeEvent(QCloseEvent *e) override;

private slots:
    void setEdited(bool val);

    void showJobContextMenu(const QPoint &pos);
    void showStopsContextMenu(const QPoint &pos);

    void onStopIndexClicked(const QModelIndex &index);

    void onJobRemoved(db_id jobId);

    void startJobNumberTimer();
    void onJobIdChanged(db_id jobId);

    void onCategoryChanged(int newCat);

    void onJobShiftChanged(db_id shiftId);

private:
    void setSpinColor(const QColor &col);

    bool setJob_internal(db_id jobId);

    void stopJobNumberTimer();
    void checkJobNumberValid();

private:
    Ui::JobPathEditor *ui;

    CustomCompletionLineEdit *shiftCombo;

    StopModel *stopModel;
    StopDelegate *delegate;

    NextPrevRSJobsModel *prevJobsModel;
    NextPrevRSJobsModel *nextJobsModel;

    int jobNumberTimerId;

    // TODO: there are too many bools
    bool isClear;

    bool canSetJob; // TODO: better name
    bool m_readOnly;
};

#endif // JOBPATHEDITOR_H
