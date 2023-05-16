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

#ifndef LOADSQLITETASK_H
#define LOADSQLITETASK_H

#include "../loadtaskutils.h"

/* LoadSQLiteTask
 *
 * Loads rollingstock pieces/models/owners from
 * another TrainTimetable Session (*ttt, *.db, other SQLite extensions)
 */
class LoadSQLiteTask : public ILoadRSTask
{
public:
    //5 steps
    //Load DB, Load Owners, Load Models, Load RS, Unselect Owners
    enum { StepSize = 100, MaxProgress = 5 * StepSize };

    LoadSQLiteTask(sqlite3pp::database &db, int mode, const QString& fileName, QObject *receiver);

    void run() override;

private:
    void endWithDbError(const QString& text);

    inline int calcProgress() const { return currentProgress + localProgress / localCount * StepSize; }

    bool attachDB();
    bool copyOwners();
    bool copyModels();
    bool copyRS();
    bool unselectOwnersWithNoRS();

private:
    const int importMode;
    int currentProgress;
    int localCount;
    int localProgress;
};

#endif // LOADSQLITETASK_H
