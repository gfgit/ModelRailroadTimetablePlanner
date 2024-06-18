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

#ifndef RSWORKER_H
#define RSWORKER_H

#ifdef ENABLE_BACKGROUND_MANAGER

#    include "utils/thread/iquittabletask.h"
#    include "utils/thread/taskprogressevent.h"

#    include <QMap>
#    include <QList>

#    include "rs_error_data.h"

namespace sqlite3pp {
class database;
class query;
} // namespace sqlite3pp

class RsErrWorker : public IQuittableTask
{
public:
    RsErrWorker(sqlite3pp::database &db, QObject *receiver, const QList<db_id> &vec);

    void run() override;

private:
    void checkRs(RsErrors::RSErrorList &rs, sqlite3pp::query &q_selectCoupling);
    void finish(const QMap<db_id, RsErrors::RSErrorList> &results, bool merge);

private:
    sqlite3pp::database &mDb;

    QList<db_id> rsToCheck;
};

class RsWorkerResultEvent : public GenericTaskEvent
{
public:
    static const Type _Type = Type(CustomEvents::RsErrWorkerResult);

    RsWorkerResultEvent(RsErrWorker *worker, const QMap<db_id, RsErrors::RSErrorList> &data,
                        bool merge);
    ~RsWorkerResultEvent();

    QMap<db_id, RsErrors::RSErrorList> results;
    bool mergeErrors;
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // RSWORKER_H
