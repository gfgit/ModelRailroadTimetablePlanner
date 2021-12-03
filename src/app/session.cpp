#include "app/session.h"
#include "info.h"

#include <QCoreApplication>

#include <QDebug>
#include "app/scopedebug.h"

#include "viewmanager/viewmanager.h"
#include "db_metadata/metadatamanager.h"

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

    m_Db(nullptr)
{
    session = this; //Global singleton pointer

    QString settings_file;
    if(qApp->arguments().contains("test"))
    {
        //If testing use exe folder instead of AppData
        settings_file = QCoreApplication::applicationDirPath() + QStringLiteral("/mrtp_settings.ini");
    }

    loadSettings(settings_file);

    viewManager.reset(new ViewManager);

    metaDataMgr.reset(new MetaDataManager(m_Db));

#ifdef ENABLE_BACKGROUND_MANAGER
    backgroundManager.reset(new BackgroundManager);
#endif
}

MeetingSession::~MeetingSession()
{

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

    //Close all graphs
    viewManager->clearAllLineGraphs();

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

    //Tables
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
                          "name TEXT NOT NULL UNIQUE,"
                          "short_name TEXT UNIQUE,"
                          "type INTEGER NOT NULL,"
                          "phone_number INTEGER UNIQUE,"
                          "svg_data BLOB,"
                          "CHECK(length(name)>0) )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE station_tracks ("
                          "id INTEGER PRIMARY KEY,"
                          "station_id INTEGER NOT NULL,"
                          "pos INTEGER NOT NULL,"
                          "type INTEGER NOT NULL,"
                          "track_length_cm INTEGER NOT NULL,"
                          "platf_length_cm INTEGET NOT NULL,"
                          "freight_length_cm INTEGER NOT NULL,"
                          "max_axes INTEGER NOT NULL,"
                          "color_rgb INTEGER,"
                          "name TEXT NOT NULL,"
                          "CHECK("
                          " length(name)>0 AND max_axes>=2 AND track_length_cm>0"
                          " AND (platf_length_cm BETWEEN 0 AND track_length_cm)"
                          " AND (freight_length_cm BETWEEN 0 AND track_length_cm)"
                          "),"
                          "UNIQUE(station_id, pos),"
                          "UNIQUE(station_id, name),"
                          "FOREIGN KEY (station_id) REFERENCES stations(id) ON UPDATE CASCADE ON DELETE CASCADE )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE station_gates ("
                          "id INTEGER PRIMARY KEY,"
                          "station_id INTEGER NOT NULL,"
                          "out_track_count INTEGER NOT NULL,"
                          "type INTEGER NOT NULL,"
                          "def_in_platf_id INTEGER,"
                          "name TEXT NOT NULL,"
                          "side INTEGER NOT NULL,"
                          "CHECK("
                          " out_track_count>0 AND (type&(1<<0) OR type&(1<<1))"
                          " AND (length(name)=1 AND name BETWEEN 'A' AND 'Z')"
                          ")," //NOTE: see utils::GateType
                          "FOREIGN KEY (station_id) REFERENCES stations(id) ON UPDATE CASCADE ON DELETE CASCADE,"
                          "FOREIGN KEY(def_in_platf_id) REFERENCES station_tracks(id) ON UPDATE CASCADE ON DELETE SET NULL,"
                          "UNIQUE(station_id,name) )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE station_gate_connections ("
                          "id INTEGER PRIMARY KEY,"
                          "track_id INTEGER NOT NULL,"
                          "track_side INTEGER NOT NULL,"
                          "gate_id INTEGER NOT NULL,"
                          "gate_track INTEGER NOT NULL,"
                          "UNIQUE(gate_id,track_id,track_side,gate_track),"
                          "FOREIGN KEY (track_id) REFERENCES station_tracks(id) ON UPDATE CASCADE ON DELETE CASCADE,"
                          "FOREIGN KEY (gate_id) REFERENCES station_gates(id) ON UPDATE CASCADE ON DELETE CASCADE )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE railway_segments ("
                          "id INTEGER PRIMARY KEY,"
                          "in_gate_id INTEGER NOT NULL,"
                          "out_gate_id INTEGER NOT NULL,"
                          "name TEXT,"
                          "max_speed_kmh INTEGER NOT NULL,"
                          "type INTEGER NOT NULL,"
                          "distance_meters INTEGER NOT NULL,"
                          "UNIQUE(in_gate_id),"
                          "UNIQUE(out_gate_id),"
                          "FOREIGN KEY(in_gate_id) REFERENCES station_gates(id) ON UPDATE CASCADE ON DELETE CASCADE,"
                          "FOREIGN KEY(out_gate_id) REFERENCES station_gates(id) ON UPDATE CASCADE ON DELETE CASCADE,"
                          "CHECK(in_gate_id<>out_gate_id AND"
                          "      max_speed_kmh>=10 AND"
                          "      distance_meters>=100 AND"
                          "      length(name)>0) )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE railway_connections ("
                          "id INTEGER PRIMARY KEY,"
                          "seg_id INTEGER NOT NULL,"
                          "in_track INTEGER NOT NULL,"
                          "out_track INTEGER NOT NULL,"
                          "UNIQUE(seg_id,in_track),"
                          "UNIQUE(seg_id,out_track),"
                          "FOREIGN KEY(seg_id) REFERENCES railway_segments(id) ON UPDATE CASCADE ON DELETE RESTRICT )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE lines ("
                          "id INTEGER PRIMARY KEY,"
                          "name TEXT NOT NULL UNIQUE,"
                          "start_meters INTEGER NOT NULL DEFAULT 0 )");
    CHECK(result);

    result =  m_Db.execute("CREATE TABLE line_segments ("
                          "id INTEGER PRIMARY KEY,"
                          "line_id INTEGER NOT NULL,"
                          "seg_id INTEGER NOT NULL,"
                          "direction INTEGER NOT NULL,"
                          "pos INTEGER NOT NULL,"
                          "FOREIGN KEY(line_id) REFERENCES lines(id) ON UPDATE CASCADE ON DELETE CASCADE,"
                          "FOREIGN KEY(seg_id) REFERENCES railway_segments(id) ON UPDATE CASCADE ON DELETE RESTRICT,"
                          "UNIQUE(line_id, seg_id)"
                          "UNIQUE(line_id, pos)"
                          "CHECK(pos<100) )"); //Allow up to 100 segments for each line
    CHECK(result);

    result = m_Db.execute("CREATE TABLE jobshifts ("
                          "id INTEGER PRIMARY KEY,"
                          "name TEXT UNIQUE NOT NULL)");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE jobs ("
                          "id INTEGER PRIMARY KEY,"
                          "category INTEGER NOT NULL DEFAULT 0,"
                          "shift_id INTEGER,"
                          "FOREIGN KEY(shift_id) REFERENCES jobshifts(id) )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE stops ("
                          "id INTEGER PRIMARY KEY,"
                          "job_id INTEGER NOT NULL,"
                          "station_id INTEGER,"
                          "arrival INTEGER NOT NULL,"
                          "departure INTEGER NOT NULL,"
                          "type INTEGER NOT NULL,"
                          "description TEXT,"

                          "in_gate_conn INTEGER,"
                          "out_gate_conn INTEGER,"
                          "next_segment_conn_id INTEGER,"

                          "CHECK(arrival<=departure),"
                          "UNIQUE(job_id,arrival),"
                          "UNIQUE(job_id,departure),"
                          "FOREIGN KEY(job_id) REFERENCES jobs(id) ON DELETE RESTRICT ON UPDATE CASCADE,"
                          "FOREIGN KEY(station_id) REFERENCES stations(id) ON DELETE RESTRICT ON UPDATE CASCADE,"
                          "FOREIGN KEY(in_gate_conn) REFERENCES station_gate_connections(id) ON UPDATE CASCADE ON DELETE RESTRICT,"
                          "FOREIGN KEY(out_gate_conn) REFERENCES station_gate_connections(id) ON UPDATE CASCADE ON DELETE RESTRICT,"
                          "FOREIGN KEY(next_segment_conn_id) REFERENCES railway_connections(id) ON UPDATE CASCADE ON DELETE RESTRICT )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE coupling ("
                          "id INTEGER PRIMARY KEY,"
                          "stop_id INTEGER,"
                          "rs_id INTEGER,"
                          "operation INTEGER NOT NULL DEFAULT 0,"

                          "FOREIGN KEY(stop_id) REFERENCES stops(id) ON DELETE CASCADE,"
                          "FOREIGN KEY(rs_id) REFERENCES rs_list(id) ON DELETE RESTRICT,"
                          "UNIQUE(stop_id,rs_id))");
    CHECK(result);

    //Create also backup tables to save old stops and couplings before editing a job and restore them if user cancels the edits.
    //NOTE: the structure of the table must be the same, remember to update theese if updating stops or couplings

    result = m_Db.execute("CREATE TABLE old_stops ("
                          "id INTEGER PRIMARY KEY,"
                          "job_id INTEGER NOT NULL,"
                          "station_id INTEGER,"
                          "arrival INTEGER NOT NULL,"
                          "departure INTEGER NOT NULL,"
                          "type INTEGER NOT NULL,"
                          "description TEXT,"

                          "in_gate_conn INTEGER,"
                          "out_gate_conn INTEGER,"
                          "next_segment_conn_id INTEGER,"

                          "CHECK(arrival<=departure),"
                          "UNIQUE(job_id,arrival),"
                          "UNIQUE(job_id,departure),"
                          "FOREIGN KEY(job_id) REFERENCES jobs(id) ON DELETE RESTRICT ON UPDATE CASCADE,"
                          "FOREIGN KEY(station_id) REFERENCES stations(id) ON DELETE RESTRICT ON UPDATE CASCADE,"
                          "FOREIGN KEY(in_gate_conn) REFERENCES station_gate_connections(id) ON UPDATE CASCADE ON DELETE RESTRICT,"
                          "FOREIGN KEY(out_gate_conn) REFERENCES station_gate_connections(id) ON UPDATE CASCADE ON DELETE RESTRICT,"
                          "FOREIGN KEY(next_segment_conn_id) REFERENCES railway_connections(id) ON UPDATE CASCADE ON DELETE RESTRICT )");
    CHECK(result);

    result = m_Db.execute("CREATE TABLE old_coupling ("
                          "id INTEGER PRIMARY KEY,"
                          "stop_id INTEGER,"
                          "rs_id INTEGER,"
                          "operation INTEGER NOT NULL DEFAULT 0,"

                          "FOREIGN KEY(stop_id) REFERENCES old_stops(id) ON DELETE CASCADE," //Old stops
                          "FOREIGN KEY(rs_id) REFERENCES rs_list(id) ON DELETE RESTRICT,"
                          "UNIQUE(stop_id,rs_id))");
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

    //Triggers

    //Prevent multiple segments on same station gate
    result = m_Db.execute("CREATE TRIGGER multiple_gate_segments\n"
                          "BEFORE INSERT ON railway_segments\n"
                          "BEGIN\n"
                          "SELECT RAISE(ABORT, 'Cannot connect same gate twice') FROM railway_segments WHERE in_gate_id=NEW.out_gate_id OR out_gate_id=NEW.in_gate_id;"
                          "END");
    CHECK(result);
    result = m_Db.execute("CREATE TRIGGER multiple_gate_segments_update_in\n"
                          "BEFORE UPDATE OF in_gate_id ON railway_segments\n"
                          "BEGIN\n"
                          "SELECT RAISE(ABORT, 'Cannot connect same gate twice') FROM railway_segments WHERE out_gate_id=NEW.in_gate_id;"
                          "END");
    CHECK(result);
    result = m_Db.execute("CREATE TRIGGER multiple_gate_segments_update_out\n"
                          "BEFORE UPDATE OF out_gate_id ON railway_segments\n"
                          "BEGIN\n"
                          "SELECT RAISE(ABORT, 'Cannot connect same gate twice') FROM railway_segments WHERE in_gate_id=NEW.out_gate_id;"
                          "END");
    CHECK(result);

    //Prevent connecting a track to a gate of a different station
    result = m_Db.execute("CREATE TRIGGER gate_conn_different_station\n"
                          "BEFORE INSERT ON station_gate_connections\n"
                          "BEGIN\n"
                          "SELECT RAISE(ABORT, 'Cannot connect platform of a different station') FROM station_tracks t"
                          " JOIN station_gates g ON g.id=NEW.gate_id"
                          " WHERE t.id=NEW.track_id AND t.station_id<>g.station_id;"
                          "END");
    CHECK(result);

    result = m_Db.execute("CREATE TRIGGER gate_conn_different_station_update\n"
                          "BEFORE UPDATE OF track_id,gate_id ON station_gate_connections\n"
                          "BEGIN\n"
                          "SELECT RAISE(ABORT, 'Cannot connect platform of a different station') FROM station_tracks t"
                          " JOIN station_gates g ON g.id=NEW.gate_id"
                          " WHERE t.id=NEW.track_id AND t.station_id<>g.station_id;"
                          "END");
    CHECK(result);

    //FIXME: Remote possibility of updating 'station_id' of track or gate.

    //Prevent connecting gate track out of bound
    result = m_Db.execute("CREATE TRIGGER gate_conn_gate_track_bound\n"
                          "BEFORE INSERT ON station_gate_connections\n"
                          "BEGIN\n"
                          "SELECT RAISE(ABORT, 'Gate track out of bound') FROM station_gates g"
                          " WHERE g.id=NEW.gate_id AND g.out_track_count<=NEW.gate_track;"
                          "END");
    CHECK(result);

    result = m_Db.execute("CREATE TRIGGER gate_conn_gate_track_bound_update\n"
                          "BEFORE UPDATE OF gate_id,gate_track ON station_gate_connections\n"
                          "BEGIN\n"
                          "SELECT RAISE(ABORT, 'Gate track out of bound') FROM station_gates g"
                          " WHERE g.id=NEW.gate_id AND g.out_track_count<=NEW.gate_track;"
                          "END");
    CHECK(result);

    result = m_Db.execute("CREATE TRIGGER gate_out_track_bound_update\n"
                          "BEFORE UPDATE OF out_track_count ON station_gates\n"
                          "BEGIN\n"
                          "SELECT RAISE(ABORT, 'Cannot remove gate tracks. Platforms connected.') FROM station_gate_connections c"
                          " WHERE c.gate_id=NEW.id AND NEW.out_track_count<=c.gate_track;"
                          "END");
    CHECK(result);

    //Prevent setting gate default track to a track which is not connected to it
    result = m_Db.execute("CREATE TRIGGER gate_def_platf_not_connected\n"
                          "BEFORE INSERT ON station_gates\n"
                          "BEGIN\n"
                          "SELECT RAISE(ABORT, 'Platform not connected to this gate') WHERE"
                          " NEW.def_in_platf_id NOT NULL AND NOT EXISTS ("
                          " SELECT 1 FROM station_gate_connections WHERE track_id=NEW.def_in_platf_id AND gate_id=NEW.id"
                          ");"
                          "END");
    CHECK(result);

    result = m_Db.execute("CREATE TRIGGER gate_def_platf_not_connected_update\n"
                          "BEFORE UPDATE OF def_in_platf_id ON station_gates\n"
                          "BEGIN\n"
                          "SELECT RAISE(ABORT, 'Platform not connected to this gate') WHERE"
                          " NEW.def_in_platf_id NOT NULL AND NOT EXISTS ("
                          " SELECT 1 FROM station_gate_connections WHERE track_id=NEW.def_in_platf_id AND gate_id=NEW.id"
                          ");"
                          "END");
    CHECK(result);

//FIXME: if setting default gate track but then delete track connection -> invalid state


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

    //FIXME: remove queries from ViewManager
    viewManager->prepareQueries();
}

void MeetingSession::finalizeStatements()
{
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
    for(const QString& point : qAsConst(savepointList))
    {
        if(!revertToSavepoint(point))
            return false;
    }
    return true;
}

QColor MeetingSession::colorForCat(JobCategory cat)
{
    QColor col = settings.getCategoryColor(int(cat)); //TODO: maybe session-specific
    if(col.isValid())
        return col;
    return QColor(Qt::gray); //Error
}

void MeetingSession::locateAppdata()
{
    appDataPath = QStringLiteral("%1/%2/%3")
                      .arg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
                      .arg(AppCompany)
                      .arg(AppProductShort);
    appDataPath = QDir::cleanPath(appDataPath);
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
