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

#ifndef LOADTASKUTILS_H
#define LOADTASKUTILS_H

#include <QCoreApplication>
#include "utils/thread/iquittabletask.h"
#include "importmodes.h"

namespace sqlite3pp {
class database;
}

class ILoadRSTask : public IQuittableTask
{
public:
    ILoadRSTask(sqlite3pp::database &db, const QString &fileName, QObject *receiver);

    inline QString getErrorText() const
    {
        return errText;
    }

protected:
    sqlite3pp::database &mDb;

    QString mFileName;
    QString errText;
};

class LoadTaskUtils
{
    Q_DECLARE_TR_FUNCTIONS(LoadTaskUtils)
public:
    enum
    {
        BatchSize = 50
    };
};

#endif // LOADTASKUTILS_H
