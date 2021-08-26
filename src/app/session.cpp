#include "app/session.h"
#include "info.h"
#include "utils/platform_utils.h"

#include <QCoreApplication>

#include <QDebug>
#include "app/scopedebug.h"

#include "viewmanager/viewmanager.h"
#include "graph/graphmanager.h"
#include "db_metadata/metadatamanager.h"

#include "lines/linestorage.h"

#include "jobs/jobstorage.h"

#ifdef ENABLE_BACKGROUND_MANAGER
#include "backgroundmanager/backgroundmanager.h"

#ifdef ENABLE_RS_CHECKER
#include "rollingstock/rs_checker/rscheckermanager.h"
#endif // ENABLE_RS_CHECKER

#endif // ENABLE_BACKGROUND_MANAGER

#include "utils/jobcategorystrings.h"

#include <QStandardPaths>
#include <QDir>

MeetingSession* MeetingSession::session;
QString MeetingSession::appDataPath;

MeetingSession::MeetingSession() :
    hourOffset(140),
    stationOffset(150),
    platformOffset(15),

    horizOffset(50),
    vertOffset(30),

    jobLineWidth(6),

    m_Db(nullptr),

    q_getPrevStop(m_Db),
    q_getNextStop(m_Db),

    q_getKmDirection(m_Db)
{
    session = this; //Global singleton pointer

    QString settings_file;
    if(qApp->arguments().contains("test"))
    {
        //If testing use exe folder instead of AppData
        settings_file = QCoreApplication::applicationDirPath() + QStringLiteral("/mrtp_settings.ini");
    }

    loadSettings(settings_file);

    mLineStorage = new LineStorage(m_Db);
    mJobStorage = new JobStorage(m_Db);

    viewManager.reset(new ViewManager);

    metaDataMgr.reset(new MetaDataManager(m_Db));

#ifdef ENABLE_BACKGROUND_MANAGER
    backgroundManager.reset(new BackgroundManager);
#endif
}

MeetingSession::~MeetingSession()
{
    //JobStorage refers to LineStorage so delete it first
    delete mJobStorage;
    delete mLineStorage;
}

MeetingSession *MeetingSession::Get()
{
    return session;
}

DB_Error MeetingSession::openDB(const QString &str, bool ignoreVersion)
{
    DEBUG_ENTRY;

    DB_Error err = closeDB();
    if(err != DB_Error::NoError && err != DB_Error::DbNotOpen)
    {
        return err;
    }

    //try{
    if(m_Db.connect(str.toUtf8(), SQLITE_OPEN_READWRITE) != SQLITE_OK)
    {
        //throw database_error(m_Db);
        qWarning() << "DB:" << m_Db.error_msg();
        return DB_Error::GenericError;
    }

    if(!ignoreVersion)
    {
        qint64 version = 0;
        switch (metaDataMgr->getInt64(version, MetaDataKey::FormatVersionKey))
        {
        case MetaDataKey::Result::ValueFound:
        {
            if(version < FormatVersion)
                return DB_Error::FormatTooOld;
            else if(version > FormatVersion)
                return DB_Error::FormatTooNew;
            break;
        }
        default:
            return DB_Error::FormatTooOld;
        }
    }

    fileName = str;

    //Enable foreign keys to ensure no invalid operation is allowed
    m_Db.enable_foreign_keys(true);
    m_Db.enable_extended_result_codes(true);

    prepareQueryes();

    mJobStorage->fixJobs();

    //    }catch(const char *msg)
    //    {
    //        QMessageBox::warning(nullptr,
    //                             QObject::tr("Error"),
    //                             QObject::tr("Error while opening file:\n%1\n'%2'")
    //                             .arg(str)
    //                             .arg(msg));
    //        throw;
    //        return false;
    //    }
    //    catch(std::exception& e)
    //    {
    //        QMessageBox::warning(nullptr,
    //                             QObject::tr("Error"),
    //                             QObject::tr("Error while opening file:\n%1\n'%2'")
    //                             .arg(str)
    //                             .arg(e.what()));
    //        throw;
    //        return false;
    //    }
    //    catch(...)
    //    {
    //        QMessageBox::warning(nullptr,
    //                             QObject::tr("Error"),
    //                             QObject::tr("Unknown error while opening file:\n%1").arg(str));
    //        throw;
    //        return false;
    //    }

#ifdef ENABLE_RS_CHECKER
    if(settings.getCheckRSWhenOpeningDB())
        backgroundManager->getRsChecker()->startWorker();
#endif


    return DB_Error::NoError;
}

DB_Error MeetingSession::closeDB()
{
    DEBUG_ENTRY;

    if(!m_Db.db())
        return DB_Error::DbNotOpen;

#ifdef SEARCHBOX_MODE_ASYNC
    backgroundManager->abortTrivialTasks();
#endif

#ifdef ENABLE_BACKGROUND_MANAGER
    backgroundManager->abortAllTasks();
#endif

    if(!viewManager->closeEditors())
        return DB_Error::EditorsStillOpened; //User wants to continue editing

    releaseAllSavepoints();

    finalizeStatements();


    //Calls sqlite3_close(), not forcing closing db like sqlite3_close_v2
    //So in case the database is still used by some background task (returns SQLITE_BUSY)
    //we abort closing and return. It's like nevere having closed, database is 100% working
    int rc = m_Db.disconnect();
    if(rc != SQLITE_OK)
    {
        qWarning() << "Err: closing db" << m_Db.error_code() << m_Db.error_msg();

        if(rc == SQLITE_BUSY)
        {
            prepareQueryes(); //Try to reprepare queries
            return DB_Error::DbBusyWhenClosing;
        }
        //return false;
    }

    mJobStorage->clear();
    mLineStorage->clear();

    //Clear current line
    getViewManager()->getGraphMgr()->setCurrentLine(0);

#ifdef ENABLE_RS_CHECKER
    backgroundManager->getRsChecker()->clearModel();
#endif

    fileName.clear();

    return DB_Error::NoError;
}

DB_Error MeetingSession::createNewDB(const QString& file)
{
    DEBUG_ENTRY;
    DB_Error err = closeDB();
    if(err != DB_Error::NoError && err != DB_Error::DbNotOpen)
    {
        return err;
    }

    int result = SQLITE_OK;
#define CHECK(code) if((code) != SQLITE_OK) qWarning() << __LINE__ << (code) << m_Db.error_code() << m_Db.error_msg()

    result = m_Db.connect(file.toUtf8(), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    CHECK(result);

    //See 'openDB()'
    m_Db.enable_foreign_keys(true);
    m_Db.enable_extended_result_codes(true);

    fileName = file;

    result = m_Db.execute("CREATE TABLE rs_models ("
                          "id INTEGER,"
                          "name TEXT,"
                          "suffix TEXT NOT NULL,"
                          "max_speed INTEGER,"
                          "axes INTEGER,"
                          "type INTEGER NOT NULL,"
                          "sub_type INTEGER NOT NULL,"
                          "PRIMARY KEY(id),"
                          "UNIQUE(name,suffix) )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE rs_owners ("
                          " id INTEGER,"
                          " name TEXT UNIQUE,"
                          " PRIMARY KEY(id) )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE rs_list ("
                          "id       INTEGER,"
                          "model_id INTEGER,"
                          "number   INTEGER NOT NULL,"
                          "owner_id INTEGER,"

                          "FOREIGN KEY(model_id) REFERENCES rs_models(id) ON DELETE RESTRICT,"
                          "FOREIGN KEY(owner_id) REFERENCES rs_owners(id) ON DELETE RESTRICT,"
                          "PRIMARY KEY(id),"
                          "UNIQUE(model_id,number,owner_id) )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE stations ("
                          "id INTEGER PRIMARY KEY,"
                          "name TEXT UNIQUE,"
                          "short_name TEXT UNIQUE,"
                          "platforms INTEGER NOT NULL,"
                          "depot_platf INTEGER,"  //depot can be 0
                          "platf_color INTEGER,"
                          "defplatf_freight INTEGER,"
                          "defplatf_passenger INTEGER )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE lines ("
                          "id INTEGER PRIMARY KEY,"
                          "name TEXT NOT NULL UNIQUE,"
                          "max_speed INTEGER DEFAULT 120,"
                          "type INTEGER )");
    CHECK(result);

    result =  m_Db.execute("CREATE TABLE railways ("
                           "id INTEGER PRIMARY KEY,"
                           "lineId INTEGER NOT NULL,"
                           "stationId INTEGER NOT NULL,"
                           "pos_meters INTEGER,"
                           "direction INTEGER DEFAULT 0,"

                           "UNIQUE(lineId, stationId)"
                           "UNIQUE(lineId, pos_meters)"
                           "FOREIGN KEY(lineId) REFERENCES lines(id) ON DELETE CASCADE,"
                           "FOREIGN KEY(stationId) REFERENCES stations(id) ON DELETE RESTRICT)"); //Delete lines but block when deleting stations registered in line
    CHECK(result);

    result = m_Db.execute("CREATE TABLE jobshifts ("
                          "id INTEGER PRIMARY KEY,"
                          "name TEXT UNIQUE NOT NULL)");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE jobs ("
                          "id INTEGER NOT NULL,"
                          "category INTEGER NOT NULL DEFAULT 0,"
                          "firstStop INTEGER,"
                          "lastStop INTEGER,"
                          "shiftId INTEGER,"
                          "PRIMARY KEY(id),"

                          "FOREIGN KEY(firstStop) REFERENCES stops(id) ON DELETE SET NULL,"
                          "FOREIGN KEY(lastStop) REFERENCES stops(id) ON DELETE SET NULL,"
                          "FOREIGN KEY(shiftId) REFERENCES jobshifts(id) )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE jobsegments ("
                          "id INTEGER,"
                          "jobId INTEGER,"
                          "lineId INTEGER,"
                          "num INTEGER,"
                          "PRIMARY KEY(id),"
                          "FOREIGN KEY(jobId) REFERENCES jobs(id) ON DELETE RESTRICT ON UPDATE CASCADE,"
                          "FOREIGN KEY(lineId) REFERENCES lines(id) ON DELETE RESTRICT,"
                          "UNIQUE(jobId,num))");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE stops ("
                          "id INTEGER PRIMARY KEY,"
                          "jobId INTEGER,"
                          "stationId INTEGER,"
                          "arrival INTEGER,"
                          "departure INTEGER,"
                          "platform INTEGER NOT NULL DEFAULT 1,"
                          "transit INTEGER DEFAULT 0,"
                          "description TEXT,"
                          "segmentId INTEGER,"
                          "otherSegment INTEGER,"
                          "rw_node INTEGER,"
                          "other_rw_node INTEGER,"

                          "FOREIGN KEY(otherSegment) REFERENCES jobsegments(id) ON DELETE RESTRICT,"
                          "FOREIGN KEY(segmentId) REFERENCES jobsegments(id) ON DELETE RESTRICT,"
                          "FOREIGN KEY(jobId) REFERENCES jobs(id) ON DELETE RESTRICT ON UPDATE CASCADE,"
                          "FOREIGN KEY(stationId) REFERENCES stations(id) ON DELETE RESTRICT,"
                          "FOREIGN KEY(rw_node) REFERENCES railways(id) ON DELETE RESTRICT,"
                          "FOREIGN KEY(other_rw_node) REFERENCES railways(id) ON DELETE RESTRICT,"
                          "UNIQUE(jobId,arrival),"
                          "UNIQUE(jobId,departure))");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE coupling ("
                          "id INTEGER PRIMARY KEY,"
                          "stopId INTEGER,"
                          "rsId INTEGER,"
                          "operation INTEGER DEFAULT 0,"

                          "FOREIGN KEY(stopId) REFERENCES stops(id) ON DELETE CASCADE,"
                          "FOREIGN KEY(rsId) REFERENCES rs_list(id) ON DELETE RESTRICT,"
                          "UNIQUE(stopId,rsId))");
    CHECK(result);

    //Create also backup tables to save old jobsegments stops and couplings before editing a job and restore them if user cancels the edits.
    //NOTE: the structure of the table must be the same, remember to update theese if updating jobsegments stops or coupling
    result = m_Db.execute("CREATE TABLE old_jobsegments ("
                          "id INTEGER,"
                          "jobId INTEGER,"
                          "lineId INTEGER,"
                          "num INTEGER,"
                          "PRIMARY KEY(id),"
                          "FOREIGN KEY(jobId) REFERENCES jobs(id) ON DELETE RESTRICT ON UPDATE CASCADE,"
                          "FOREIGN KEY(lineId) REFERENCES lines(id) ON DELETE RESTRICT,"
                          "UNIQUE(jobId,num) )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE old_stops ("
                          "id INTEGER PRIMARY KEY,"
                          "jobId INTEGER,"
                          "stationId INTEGER,"
                          "arrival INTEGER,"
                          "departure INTEGER,"
                          "platform INTEGER NOT NULL DEFAULT 1,"
                          "transit INTEGER DEFAULT 0,"
                          "description TEXT,"
                          "segmentId INTEGER,"
                          "otherSegment INTEGER,"
                          "rw_node INTEGER,"
                          "other_rw_node INTEGER,"

                          "FOREIGN KEY(otherSegment) REFERENCES old_jobsegments(id) ON DELETE RESTRICT," //NOTE: this must refer to 'old_jobsegments' instead of 'jobsegments'
                          "FOREIGN KEY(segmentId) REFERENCES old_jobsegments(id) ON DELETE RESTRICT,"    //NOTE: this must refer to 'old_jobsegments' instead of 'jobsegments'
                          "FOREIGN KEY(jobId) REFERENCES jobs(id) ON DELETE RESTRICT ON UPDATE CASCADE,"
                          "FOREIGN KEY(stationId) REFERENCES stations(id) ON DELETE RESTRICT,"
                          "FOREIGN KEY(rw_node) REFERENCES railways(id) ON DELETE RESTRICT,"
                          "FOREIGN KEY(other_rw_node) REFERENCES railways(id) ON DELETE RESTRICT,"
                          "UNIQUE(jobId,arrival),"
                          "UNIQUE(jobId,departure) )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE old_coupling ("
                          "id INTEGER PRIMARY KEY,"
                          "stopId INTEGER,"
                          "rsId INTEGER,"
                          "operation INTEGER DEFAULT 0,"

                          "FOREIGN KEY(stopId) REFERENCES old_stops(id) ON DELETE CASCADE," //NOTE: this must refer to 'old_stops' instead of 'stops'
                          "FOREIGN KEY(rsId) REFERENCES rs_list(id) ON DELETE RESTRICT,"
                          "UNIQUE(stopId,rsId))");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE imported_rs_owners ("
                          "id INTEGER,"
                          "name TEXT,"
                          "import INTEGER,"
                          "new_name TEXT,"
                          "match_existing_id INTEGER,"
                          "sheet_idx INTEGER,"
                          "PRIMARY KEY(id),"
                          "FOREIGN KEY(match_existing_id) REFERENCES rs_owners(id) ON UPDATE RESTRICT ON DELETE RESTRICT)");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE imported_rs_models ("
                          "id INTEGER,"
                          "name TEXT,"
                          "suffix TEXT NOT NULL,"
                          "import INTEGER,"
                          "new_name TEXT,"
                          "match_existing_id INTEGER,"
                          "max_speed INTEGER,"
                          "axes INTEGER,"
                          "type INTEGER,"
                          "sub_type INTEGER,"
                          "PRIMARY KEY(id),"
                          "FOREIGN KEY(match_existing_id) REFERENCES rs_models(id) ON UPDATE RESTRICT ON DELETE RESTRICT)");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE imported_rs_list ("
                          "id INTEGER,"
                          "import INTEGER,"
                          "model_id INTEGER,"
                          "owner_id INTEGER,"
                          "number INTEGER,"
                          "new_number INTEGER,"
                          "PRIMARY KEY(id),"
                          "FOREIGN KEY(model_id) REFERENCES imported_rs_models(id) ON UPDATE RESTRICT ON DELETE RESTRICT,"
                          "FOREIGN KEY(owner_id) REFERENCES imported_rs_owners(id) ON UPDATE RESTRICT ON DELETE RESTRICT)");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE metadata ("
                          "name TEXT PRIMARY KEY,"
                          "val BLOB)");
    CHECK(result);
#undef CHECK

    metaDataMgr->setInt64(FormatVersion, false, MetaDataKey::FormatVersionKey);
    metaDataMgr->setString(AppVersion, false, MetaDataKey::ApplicationString);

    prepareQueryes();

    return DB_Error::NoError;
}

/* bool MeetingSession::checkImportRSTablesEmpty()
 * Check if import_rs_list, import_rs_models, import_rs_owners tables are empty
 * Theese tables are used during RS importation and are cleared when the process
 * completes or gets canceled by the user
 * If they are not empty it might be because the application crashed before clearing theese tables
*/
bool MeetingSession::checkImportRSTablesEmpty()
{
    query q(m_Db, "SELECT COUNT(1) FROM imported_rs_list");
    q.step();
    int count = q.getRows().get<int>(0);
    if(count)
        return false;

    q.prepare("SELECT COUNT(1) FROM imported_rs_models");
    q.step();
    count = q.getRows().get<int>(0);
    if(count)
        return false;

    q.prepare("SELECT COUNT(1) FROM imported_rs_owners");
    q.step();
    count = q.getRows().get<int>(0);
    if(count)
        return false;
    return true;
}

bool MeetingSession::clearImportRSTables()
{
    command cmd(m_Db, "DELETE FROM imported_rs_list");
    if(cmd.execute() != SQLITE_OK)
        return false;

    cmd.prepare("DELETE FROM imported_rs_models");
    if(cmd.execute() != SQLITE_OK)
        return false;

    cmd.prepare("DELETE FROM imported_rs_owners");
    if(cmd.execute() != SQLITE_OK)
        return false;

    return true;
}

void MeetingSession::prepareQueryes()
{
    DEBUG_COLOR_ENTRY(SHELL_YELLOW);

    if(q_getPrevStop.prepare("SELECT MAX(prev.departure),"
                             "prev.stationId,"
                             "seg.lineId"
                             " FROM stops prev"
                             " JOIN stops s ON s.jobId=prev.jobId AND prev.departure<s.arrival"
                             " JOIN jobsegments seg ON seg.id=s.segmentId"
                             " WHERE s.id=?") != SQLITE_OK)
    {
        throw database_error(m_Db);
    }

    if(q_getNextStop.prepare("SELECT MIN(nextS.arrival),"
                             "nextS.stationId,"
                             "seg.lineId"
                             " FROM stops nextS"
                             " JOIN stops s ON s.jobId=nextS.jobId AND nextS.arrival>s.departure"
                             " JOIN jobsegments seg ON seg.id=nextS.segmentId"
                             " WHERE s.id=?") != SQLITE_OK)
    {
        throw database_error(m_Db);
    }

    if(q_getKmDirection.prepare("SELECT pos_meters, direction FROM railways WHERE lineId=? AND stationId=?") != SQLITE_OK)
    {
        throw database_error(m_Db);
    }

    viewManager->prepareQueries();
}

void MeetingSession::finalizeStatements()
{
    q_getPrevStop.finish();
    q_getNextStop.finish();

    q_getKmDirection.finish();

    viewManager->finalizeQueries();
}

bool MeetingSession::setSavepoint(const QString &pointname)
{
    if(!m_Db.db())
        return false;

    if(savepointList.contains(pointname))
        return true;

    QString sql = QStringLiteral("SAVEPOINT %1;");

    if(m_Db.execute(sql.arg(pointname).toUtf8()) != SQLITE_OK)
    {
        qDebug() << m_Db.error_msg();
        return false;
    }
    savepointList.append(pointname);

    return true;
}

bool MeetingSession::releaseSavepoint(const QString &pointname)
{
    if(!m_Db.db())
        return false;

    if(!savepointList.contains(pointname))
        return true;

    QString sql = QStringLiteral("RELEASE %1;");

    if(m_Db.execute(sql.arg(pointname).toUtf8()) != SQLITE_OK)
    {
        qDebug() << m_Db.error_msg();
        return false;
    }

    int point_index = savepointList.lastIndexOf(pointname);
    savepointList.erase(savepointList.begin() + point_index, savepointList.end());

    return true;
}

bool MeetingSession::revertToSavepoint(const QString &pointname)
{
    if(!m_Db.db())
        return false;

    if(!savepointList.contains(pointname))
        return false;

    QString sql = QStringLiteral("ROLLBACK TO SAVEPOINT %1;");

    if(m_Db.execute(sql.arg(pointname).toUtf8()) != SQLITE_OK)
    {
        qDebug() << m_Db.error_msg();
        return false;
    }

    int point_index = savepointList.lastIndexOf(pointname);
    savepointList.erase(savepointList.begin() + point_index, savepointList.end());

    return true;
}

bool MeetingSession::releaseAllSavepoints()
{
    if(!m_Db.db())
        return false;

    for(const QString& point : qAsConst(savepointList))
    {
        if(!releaseSavepoint(point))
            return false;
    }

    // When still in a transaction, commit that too
    if(sqlite3_get_autocommit(m_Db.db()) == 0)
        m_Db.execute("COMMIT;");

    return true;
}

bool MeetingSession::revertAll()
{
    for(const QString& point : savepointList)
    {
        if(!revertToSavepoint(point))
            return false;
    }
    return true;
}

qreal MeetingSession::getStationGraphPos(db_id lineId, db_id stId, int platf)
{
    qreal x = horizOffset;

    query q(Session->m_Db, "SELECT s.id,s.platforms,s.depot_platf FROM railways"
                           " JOIN stations s ON s.id=railways.stationId"
                           " WHERE railways.lineId=? ORDER BY railways.pos_meters ASC");
    q.bind(1, lineId);

    for(auto station : q)
    {
        if(station.get<db_id>(0) == stId)
            return x + platf * platformOffset;

        int platfCount = station.get<int>(1);
        platfCount += station.get<int>(2);

        x += stationOffset + platfCount * platformOffset;
    }
    return x;
}

QColor MeetingSession::colorForCat(JobCategory cat)
{
    QColor col = settings.getCategoryColor(int(cat)); //TODO: maybe session-specific
    if(col.isValid())
        return col;
    return QColor(Qt::gray); //Error
}

bool MeetingSession::getPrevStop(db_id stopId, db_id& prevSt, db_id& lineId)
{
    bool ret = false;
    q_getPrevStop.bind(1, stopId);
    int rc = q_getPrevStop.step();
    if(rc == SQLITE_ROW)
    {
        //MAX() always return a row but if type is NULL we ignore it
        auto r = q_getPrevStop.getRows();
        if(r.column_type(0) != SQLITE_NULL)
        {
            prevSt = r.get<db_id>(1);
            lineId = r.get<db_id>(2);
            ret = true;
        }
        else
        {
            prevSt = 0;
            lineId = 0;
        }
    }
    else
    {
        qWarning() << m_Db.error_code() << m_Db.error_msg();
    }

    q_getPrevStop.reset();
    return ret;
}

bool MeetingSession::getNextStop(db_id stopId, db_id& nextSt, db_id& lineId)
{
    bool ret = false;
    q_getNextStop.bind(1, stopId);
    int rc = q_getNextStop.step();
    if(rc == SQLITE_ROW)
    {
        //MAX() always return a row but if type is NULL we ignore it
        auto r = q_getNextStop.getRows();
        if(r.column_type(0) != SQLITE_NULL)
        {
            nextSt = r.get<db_id>(1);
            lineId = r.get<db_id>(2);
            ret = true;
        }
        else
        {
            nextSt = 0;
            lineId = 0;
        }
    }
    else
    {
        qWarning() << m_Db.error_code() << m_Db.error_msg();
    }

    q_getNextStop.reset();
    return ret;
}

Direction MeetingSession::getStopDirection(db_id stopId, db_id stId)
{
    db_id otherSt = 0;
    db_id lineId = 0;
    bool isNext = false;

    if(!getPrevStop(stopId, otherSt, lineId))
    {
        getNextStop(stopId, otherSt, lineId);
        isNext = true;
    }

    Direction dir;
    int kmInMetersA;
    int kmInMetersB;

    {
        q_getKmDirection.bind(1, lineId);
        q_getKmDirection.bind(2, stId);
        q_getKmDirection.step();
        auto r = q_getKmDirection.getRows();
        kmInMetersA = r.get<int>(0);
        dir = Direction(r.get<int>(1));
        q_getKmDirection.reset();
    }
    {
        q_getKmDirection.bind(1, lineId);
        q_getKmDirection.bind(2, otherSt);
        q_getKmDirection.step();
        auto r = q_getKmDirection.getRows();
        kmInMetersB = r.get<int>(0);
        q_getKmDirection.reset();
    }

    bool absDir = (kmInMetersA > kmInMetersB);

    bool ret = false;

    if(isNext)
        ret = !ret;

    if(absDir)
        ret = !ret;

    if(dir == Direction::Right)
        ret = !ret;

    return Direction(ret);
}

void MeetingSession::locateAppdata()
{
    appDataPath = QDir::cleanPath(QStringLiteral("%1/%2/%3"))
            .arg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
            .arg(AppCompany)
            .arg(AppDisplayName);
    qDebug() << appDataPath;
}

void MeetingSession::loadSettings(const QString& settings_file)
{
    DEBUG_ENTRY;

    if(settings_file.isEmpty())
        settings.loadSettings(appDataPath + QStringLiteral("/mrtp_settings.ini"));
    else
        settings.loadSettings(settings_file);

    hourOffset     = settings.getHourOffset();
    stationOffset  = settings.getStationOffset();
    horizOffset    = settings.getHorizontalOffset();
    vertOffset     = settings.getVerticalOffset();
    platformOffset = settings.getPlatformOffset();

    jobLineWidth = settings.getJobLineWidth();
}

#ifdef ENABLE_BACKGROUND_MANAGER
BackgroundManager* MeetingSession::getBackgroundManager() const
{
    return backgroundManager.get();
}
#endif
