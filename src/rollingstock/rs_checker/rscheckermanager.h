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

#ifndef RSCHECKERMANAGER_H
#define RSCHECKERMANAGER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#    include "backgroundmanager/ibackgroundchecker.h"

#    include "utils/types.h"

class RsCheckerManager : public IBackgroundChecker
{
    Q_OBJECT
public:
    RsCheckerManager(sqlite3pp::database &db, QObject *parent = nullptr);

    void checkRs(const QSet<db_id> &rsIds);

    QString getName() const override;
    void clearModel() override;
    void showContextMenu(QWidget *panel, const QPoint &pos, const QModelIndex &idx) const override;

    void sessionLoadedHandler() override;

public slots:
    void onRSPlanChanged(const QSet<db_id> &rsIds);

protected:
    IQuittableTask *createMainWorker() override;
    void setErrors(QEvent *e, bool merge) override;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // RSCHECKERMANAGER_H
