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

#include "importtask.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>

#include "loadprogressevent.h"
#include <QCoreApplication>

ImportTask::ImportTask(sqlite3pp::database &db, QObject *receiver) :
    IQuittableTask(receiver),
    mDb(db)
{
}

void ImportTask::run()
{
    sendEvent(new LoadProgressEvent(this, 0, 4), false);

    // FIXME: do copy in batches like LoadSQLiteTask
    // TODO: get only owners used really
    sqlite3pp::query q(mDb, "SELECT imp.id,imp.name,imp.new_name FROM imported_rs_owners imp WHERE "
                            "imp.import=1 AND imp.match_existing_id IS NULL");
    sqlite3pp::command q_create(mDb, "INSERT INTO rs_owners(id, name) VALUES(NULL, ?)");
    sqlite3pp::command q_update(mDb,
                                "UPDATE imported_rs_owners SET match_existing_id=? WHERE id=?");

    sqlite3_stmt *st     = q.stmt();
    sqlite3_stmt *create = q_create.stmt();
    sqlite3_stmt *update = q_update.stmt();

    sqlite3 *db          = mDb.db();
    sqlite3_mutex *m     = sqlite3_db_mutex(db);

    int n                = 0;
    while (q.step() == SQLITE_ROW)
    {
        if (n % 8 == 0 && wasStopped())
        {
            sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressAbortedByUser,
                                            LoadProgressEvent::ProgressMaxFinished),
                      true);
            return;
        }

        // Get imported owner id
        db_id importedOwnerId = sqlite3_column_int64(st, 0);
        int len               = 2;

        if (sqlite3_column_type(st, 2) == SQLITE_NULL)
        {
            len = 1; // Get original name instead of the custom one
        }

        // Get its name (or new_name)
        const char *name = reinterpret_cast<const char *>(sqlite3_column_text(st, len));
        len              = sqlite3_column_bytes(st, len);

        // Create a real owner with this name and get its id
        sqlite3_bind_text(create, 1, name, len, SQLITE_STATIC);
        sqlite3_mutex_enter(m);
        sqlite3_step(create);
        db_id existingOwnerId = sqlite3_last_insert_rowid(db);
        sqlite3_mutex_leave(m);
        sqlite3_reset(create);

        // Set the real owner id in the imported owner record to use it later
        sqlite3_bind_int64(update, 1, existingOwnerId);
        sqlite3_bind_int64(update, 2, importedOwnerId);
        sqlite3_step(update);
        sqlite3_reset(update);

        n++;
    }

    sendEvent(new LoadProgressEvent(this, 1, 4), false);

    q.prepare(
      "SELECT imp.id,imp.name,imp.suffix,imp.new_name,imp.max_speed,imp.axes,imp.type,imp.sub_type"
      " FROM imported_rs_models imp WHERE imp.import=1 AND imp.match_existing_id IS NULL");
    st = q.stmt();

    q_create.prepare("INSERT INTO rs_models(id,name,suffix,max_speed,axes,type,sub_type) "
                     "VALUES(NULL,?,?,?,?,?,?)"); // Create invalid engine
    create = q_create.stmt();

    q_update.prepare("UPDATE imported_rs_models SET match_existing_id=? WHERE id=?");
    update = q_update.stmt();

    n      = 0;
    while (q.step() == SQLITE_ROW)
    {
        if (n % 8 == 0 && wasStopped())
        {
            sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressAbortedByUser,
                                            LoadProgressEvent::ProgressMaxFinished),
                      true);
            return;
        }

        // Get imported model id
        db_id importedModelId = sqlite3_column_int64(st, 0);

        int len               = 3;
        if (sqlite3_column_type(st, 3) == SQLITE_NULL)
        {
            len = 1; // Get original name instead of the custom one ('name' instead of 'new_name'
        }

        // Get its name (or new_name)
        const char *name = reinterpret_cast<const char *>(sqlite3_column_text(st, len));
        len              = sqlite3_column_bytes(st, len);
        sqlite3_bind_text(create, 1, name, len, SQLITE_STATIC);

        // Get its suffix
        name = reinterpret_cast<const char *>(sqlite3_column_text(st, 2));
        len  = sqlite3_column_bytes(st, 2);
        sqlite3_bind_text(create, 2, name, len, SQLITE_STATIC);

        // Get its max_speed
        int val = sqlite3_column_int(st, 4);
        sqlite3_bind_int(create, 3, val);

        // Get its axes count
        val = sqlite3_column_int(st, 5);
        sqlite3_bind_int(create, 4, val);

        // Get its type
        val = sqlite3_column_int(st, 6);
        sqlite3_bind_int(create, 5, val);

        // Get its sub_type
        val = sqlite3_column_int(st, 7);
        sqlite3_bind_int(create, 6, val);

        // Create a real model with this name and get its id
        sqlite3_mutex_enter(m);
        sqlite3_step(create);
        db_id existingModelId = sqlite3_last_insert_rowid(db);
        sqlite3_mutex_leave(m);
        sqlite3_reset(create);

        // Set the real owner id in the imported model record to use it later
        sqlite3_bind_int64(update, 1, existingModelId);
        sqlite3_bind_int64(update, 2, importedModelId);
        sqlite3_step(update);
        sqlite3_reset(update);
        n++;
    }

    if (wasStopped())
    {
        sendEvent(new LoadProgressEvent(this, LoadProgressEvent::ProgressAbortedByUser,
                                        LoadProgressEvent::ProgressMaxFinished),
                  true);
        return;
    }
    else
    {
        sendEvent(new LoadProgressEvent(this, 2, 4), false);
    }

    // Finally import rollingstock
    q_create.prepare("INSERT INTO rs_list(id, model_id, number, owner_id)"
                     " SELECT NULL, m.match_existing_id, (CASE WHEN imp.new_number IS NULL THEN "
                     "imp.number ELSE imp.new_number END), o.match_existing_id"
                     " FROM imported_rs_list imp"
                     " JOIN imported_rs_models m ON m.id=imp.model_id "
                     " JOIN imported_rs_owners o ON o.id=imp.owner_id"
                     " WHERE imp.import=1 AND m.import=1 AND o.import=1");

    q_create.execute(); // Long running

    sendEvent(new LoadProgressEvent(this, 0, LoadProgressEvent::ProgressMaxFinished), true);
}
