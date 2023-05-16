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

#ifndef JOBCROSSINGCHECKER_H
#define JOBCROSSINGCHECKER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#include "backgroundmanager/ibackgroundchecker.h"

#include "utils/types.h"

class JobCrossingChecker : public IBackgroundChecker
{
    Q_OBJECT
public:
    JobCrossingChecker(sqlite3pp::database &db, QObject *parent = nullptr);

    QString getName() const override;
    void clearModel() override;
    void showContextMenu(QWidget *panel, const QPoint& pos, const QModelIndex& idx) const override;

    void sessionLoadedHandler() override;

protected:
    IQuittableTask *createMainWorker() override;
    void setErrors(QEvent *e, bool merge) override;

private slots:
    void onJobChanged(db_id newJobId, db_id oldJobId);
    void onJobRemoved(db_id jobId);

private:
    sqlite3pp::database &mDb;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // JOBCROSSINGCHECKER_H
