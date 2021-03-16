#include "loadsqlitetask.h"
#include "../loadprogressevent.h"

#include "../loadtaskutils.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QDebug>

LoadSQLiteTask::LoadSQLiteTask(sqlite3pp::database &db, int mode, const QString &fileName, QObject *receiver) :
    ILoadRSTask(db, fileName, receiver),
    importMode(mode)
{

}

void LoadSQLiteTask::run()
{
    currentProgress = 0;
    localCount = 0;
    localProgress = 0;

    if(wasStopped())
    {
        sendEvent(new LoadProgressEvent(this,
                                        LoadProgressEvent::ProgressAbortedByUser,
                                        LoadProgressEvent::ProgressMaxFinished),
                  true);
        return;
    }

    sendEvent(new LoadProgressEvent(this, 0, MaxProgress), false);

    if(!attachDB())
        return;

    if(!copyOwners())
        return;

    if(!copyModels())
        return;

    if(!copyRS())
        return;

    //Cleanup
    mDb.execute("DETACH rs_source");

    if(!unselectOwnersWithNoRS())
        return;

    sendEvent(new LoadProgressEvent(this,
                                    MaxProgress,
                                    LoadProgressEvent::ProgressMaxFinished),
              true);
}

void LoadSQLiteTask::endWithDbError(const QString &text)
{
    mDb.execute("DETACH rs_source");

    errText = LoadTaskUtils::tr("%1\n"
                                "Code: %2\n"
                                "Message: %3")
            .arg(text)
            .arg(mDb.extended_error_code())
            .arg(mDb.error_msg());

    sendEvent(new LoadProgressEvent(this,
                                    LoadProgressEvent::ProgressError,
                                    LoadProgressEvent::ProgressMaxFinished),
              true);
}

bool LoadSQLiteTask::attachDB()
{
    //ATTACH other database to this session
    localCount = 1;
    sqlite3pp::command q(mDb, "ATTACH ? AS rs_source"); //TODO: use URI to pass 'readonly'
    q.bind(1, mFileName);
    int ret = q.execute();
    if(ret != SQLITE_OK)
    {
        endWithDbError(LoadTaskUtils::tr("Could not open session file correctly."));
        return false;
    }

    localProgress = 0; //Advance by 1 step, clear partial progress
    currentProgress += StepSize;
    sendEvent(new LoadProgressEvent(this, calcProgress(), MaxProgress), false);
    return true;
}

bool LoadSQLiteTask::copyOwners()
{
    if((importMode & RSImportMode::ImportRSOwners) == 0)
        return true; //Skip owners importation

    sqlite3pp::query q(mDb);
    sqlite3pp::query q_getFirstIdGreaterThan(mDb);

    //Calculate maximum number of batches
    q.prepare("SELECT MAX(id) FROM rs_source.rs_owners");
    q.step();
    localCount = sqlite3_column_int(q.stmt(), 0);
    localCount = localCount/LoadTaskUtils::BatchSize + (localCount % LoadTaskUtils::BatchSize != 0); //Round up
    if(localCount < 1)
        localCount = 1; //At least 1 batch

    //Init query: get first id to import
    int ret = q_getFirstIdGreaterThan.prepare("SELECT MIN(id) FROM rs_source.rs_owners WHERE id>?");
    sqlite3_stmt *stmt = q_getFirstIdGreaterThan.stmt();
    if(ret != SQLITE_OK)
    {
        endWithDbError(LoadTaskUtils::tr("Query preparation failed A1."));
        return false;
    }

    ret = q.prepare("INSERT OR IGNORE INTO main.imported_rs_owners(id, name, import, new_name, match_existing_id, sheet_idx)"
                    " SELECT NULL,own1.name,1,NULL,own2.id,0"
                    " FROM rs_source.rs_owners AS own1"
                    " LEFT JOIN main.rs_owners own2 ON own1.name=own2.name"
                    " WHERE own1.id BETWEEN ? AND ?");
    if(ret != SQLITE_OK)
    {
        endWithDbError(LoadTaskUtils::tr("Query preparation failed A2."));
        return false;
    }

    int firstId = 0;
    localProgress = 0;
    while (true)
    {
        if(wasStopped())
        {
            q.prepare("DETACH rs_source"); //Cleanup
            q.step();

            sendEvent(new LoadProgressEvent(this,
                                            LoadProgressEvent::ProgressAbortedByUser,
                                            LoadProgressEvent::ProgressMaxFinished),
                      true);
            return false;
        }

        q_getFirstIdGreaterThan.bind(1, firstId);
        q_getFirstIdGreaterThan.step();
        if(sqlite3_column_type(stmt, 0) == SQLITE_NULL)
            break; //No more owners to import

        firstId = sqlite3_column_int(stmt, 0);
        q_getFirstIdGreaterThan.reset();

        const int lastId = firstId + LoadTaskUtils::BatchSize;
        q.bind(1, firstId);
        q.bind(2, lastId);
        firstId = lastId;

        q.step();

        localProgress++;
        sendEvent(new LoadProgressEvent(this, calcProgress(), MaxProgress), false);
    }

    localProgress = 0; //Advance by 1 step, clear partial progress
    currentProgress += StepSize;
    sendEvent(new LoadProgressEvent(this, calcProgress(), MaxProgress), false);
    return true;
}

bool LoadSQLiteTask::copyModels()
{
    if((importMode & RSImportMode::ImportRSModels) == 0)
        return true; //Skip models importation

    sqlite3pp::query q(mDb);
    sqlite3pp::query q_getFirstIdGreaterThan(mDb);

    //Calculate maximum number of batches
    q.prepare("SELECT MAX(id) FROM rs_source.rs_models");
    q.step();
    localCount = sqlite3_column_int(q.stmt(), 0);
    localCount = localCount/LoadTaskUtils::BatchSize + (localCount % LoadTaskUtils::BatchSize != 0); //Round up
    if(localCount < 1)
        localCount = 1; //At least 1 batch

    //Init query: get first id to import
    int ret = q_getFirstIdGreaterThan.prepare("SELECT MIN(id) FROM rs_source.rs_models WHERE id>?");
    sqlite3_stmt *stmt = q_getFirstIdGreaterThan.stmt();
    if(ret != SQLITE_OK)
    {
        endWithDbError(LoadTaskUtils::tr("Query preparation failed B1."));
        return false;
    }

    ret = q.prepare("INSERT OR IGNORE INTO main.imported_rs_models(id, name, suffix, import, new_name, match_existing_id, max_speed, axes, type, sub_type)"
                    " SELECT NULL,mod1.name,mod1.suffix,1,NULL,mod2.id,mod1.max_speed,mod1.axes,mod1.type,mod1.sub_type"
                    " FROM rs_source.rs_models AS mod1"
                    " LEFT JOIN main.rs_models mod2 ON mod1.name=mod2.name AND mod1.suffix=mod2.suffix"
                    " WHERE mod1.id BETWEEN ? AND ?");
    if(ret != SQLITE_OK)
    {
        endWithDbError(LoadTaskUtils::tr("Query preparation failed B2."));
        return false;
    }

    int firstId = 0;
    localProgress = 0;
    while (true)
    {
        if(wasStopped())
        {
            q.prepare("DETACH rs_source"); //Cleanup
            q.step();

            sendEvent(new LoadProgressEvent(this,
                                            LoadProgressEvent::ProgressAbortedByUser,
                                            LoadProgressEvent::ProgressMaxFinished),
                      true);
            return false;
        }

        q_getFirstIdGreaterThan.bind(1, firstId);
        q_getFirstIdGreaterThan.step();
        if(sqlite3_column_type(stmt, 0) == SQLITE_NULL)
            break; //No more models to import

        firstId = sqlite3_column_int(stmt, 0);
        q_getFirstIdGreaterThan.reset();

        const int lastId = firstId + LoadTaskUtils::BatchSize;
        q.bind(1, firstId);
        q.bind(2, lastId);
        firstId = lastId;

        q.step();

        localProgress++;
        sendEvent(new LoadProgressEvent(this, calcProgress(), MaxProgress), false);
    }

    localProgress = 0; //Advance by 1 step, clear partial progress
    currentProgress += StepSize;
    sendEvent(new LoadProgressEvent(this, calcProgress(), MaxProgress), false);
    return true;
}

bool LoadSQLiteTask::copyRS()
{
    if((importMode & RSImportMode::ImportRSPieces) == 0)
        return true; //Skip RS importation

    sqlite3pp::query q(mDb);
    sqlite3pp::query q_getFirstIdGreaterThan(mDb);

    //Calculate maximum number of batches
    q.prepare("SELECT MAX(id) FROM rs_source.rs_models");
    q.step();
    localCount = sqlite3_column_int(q.stmt(), 0);
    localCount = localCount/LoadTaskUtils::BatchSize + (localCount % LoadTaskUtils::BatchSize != 0); //Round up
    if(localCount < 1)
        localCount = 1; //At least 1 batch

    //Init query: get first id to import
    int ret = q_getFirstIdGreaterThan.prepare("SELECT MIN(id) FROM rs_source.rs_models WHERE id>?");
    sqlite3_stmt *stmt = q_getFirstIdGreaterThan.stmt();
    if(ret != SQLITE_OK)
    {
        endWithDbError(LoadTaskUtils::tr("Query preparation failed C1."));
        return false;
    }

    ret = q.prepare("INSERT OR IGNORE INTO main.imported_rs_list(id, import, model_id, owner_id, number, new_number)"
                    " SELECT NULL,1,mod2.id,own2.id,rs.number % 10000,NULL"
                    " FROM rs_source.rs_list AS rs"
                    " LEFT JOIN rs_source.rs_models mod1 ON mod1.id=rs.model_id"
                    " LEFT JOIN main.imported_rs_models mod2 ON mod1.name=mod2.name AND mod1.suffix=mod2.suffix"
                    " LEFT JOIN rs_source.rs_owners own1 ON own1.id=rs.owner_id"
                    " LEFT JOIN main.imported_rs_owners own2 ON own1.name=own2.name"
                    " WHERE mod1.id BETWEEN ? AND ?");
    if(ret != SQLITE_OK)
    {
        endWithDbError(LoadTaskUtils::tr("Query preparation failed C2."));
        return false;
    }

    int firstId = 0;
    localProgress = 0;
    while (true)
    {
        if(wasStopped())
        {
            q.prepare("DETACH rs_source"); //Cleanup
            q.step();

            sendEvent(new LoadProgressEvent(this,
                                            LoadProgressEvent::ProgressAbortedByUser,
                                            LoadProgressEvent::ProgressMaxFinished),
                      true);
            return false;
        }

        q_getFirstIdGreaterThan.bind(1, firstId);
        q_getFirstIdGreaterThan.step();
        if(sqlite3_column_type(stmt, 0) == SQLITE_NULL)
            break; //No more RS to import

        firstId = sqlite3_column_int(stmt, 0);
        q_getFirstIdGreaterThan.reset();

        const int lastId = firstId + LoadTaskUtils::BatchSize;
        q.bind(1, firstId);
        q.bind(2, lastId);
        firstId = lastId;

        q.step();

        localProgress++;
        sendEvent(new LoadProgressEvent(this, calcProgress(), MaxProgress), false);
    }

    localProgress = 0; //Advance by 1 step, clear partial progress
    currentProgress += StepSize;
    sendEvent(new LoadProgressEvent(this, calcProgress(), MaxProgress), false);
    return true;
}

bool LoadSQLiteTask::unselectOwnersWithNoRS()
{
    sqlite3pp::query q(mDb);
    sqlite3pp::query q_getFirstIdGreaterThan(mDb);

    //Calculate maximum number of batches
    q.prepare("SELECT MAX(id) FROM imported_rs_owners");
    q.step();
    localCount = sqlite3_column_int(q.stmt(), 0);
    localCount = localCount/LoadTaskUtils::BatchSize + (localCount % LoadTaskUtils::BatchSize != 0); //Round up
    if(localCount < 1)
        localCount = 1; //At least 1 batch

    //Init query: get first id to query
    int ret = q_getFirstIdGreaterThan.prepare("SELECT MIN(id) FROM imported_rs_owners WHERE id>?");
    sqlite3_stmt *stmt = q_getFirstIdGreaterThan.stmt();
    if(ret != SQLITE_OK)
    {
        endWithDbError(LoadTaskUtils::tr("Query preparation failed C1."));
        return false;
    }

    ret = q.prepare("UPDATE imported_rs_owners SET import=0 WHERE id IN("
                    " SELECT own.id"
                    " FROM imported_rs_owners own"
                    " LEFT JOIN imported_rs_list rs ON rs.owner_id=own.id"
                    " WHERE own.id BETWEEN ? AND ?"
                    " GROUP BY own.id"
                    " HAVING COUNT(rs.id)=0)");
    if(ret != SQLITE_OK)
    {
        endWithDbError(LoadTaskUtils::tr("Query preparation failed C2."));
        return false;
    }

    int firstId = 0;
    localProgress = 0;
    while (true)
    {
        if(wasStopped())
        {
            //No cleanup, already done after importing RS
            sendEvent(new LoadProgressEvent(this,
                                            LoadProgressEvent::ProgressAbortedByUser,
                                            LoadProgressEvent::ProgressMaxFinished),
                      true);
            return false;
        }

        q_getFirstIdGreaterThan.bind(1, firstId);
        q_getFirstIdGreaterThan.step();
        if(sqlite3_column_type(stmt, 0) == SQLITE_NULL)
            break; //No more owners to edit

        firstId = sqlite3_column_int(stmt, 0);
        q_getFirstIdGreaterThan.reset();

        const int lastId = firstId + LoadTaskUtils::BatchSize;
        q.bind(1, firstId);
        q.bind(2, lastId);
        firstId = lastId;

        q.step();

        localProgress++;
        sendEvent(new LoadProgressEvent(this, calcProgress(), MaxProgress), false);
    }

    localProgress = 0; //Advance by 1 step, clear partial progress
    currentProgress += StepSize;
    sendEvent(new LoadProgressEvent(this, calcProgress(), MaxProgress), false);
    return true;
}
