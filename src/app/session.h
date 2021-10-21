#ifndef MEETINGSESSION_H
#define MEETINGSESSION_H

#include "utils/types.h"
#include "utils/directiontype.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QObject>

#include <QString>

#include <QColor>

#include <settings/appsettings.h>

class LineStorage;
class JobStorage;

class ViewManager;
class MetaDataManager;

#ifdef ENABLE_BACKGROUND_MANAGER
class BackgroundManager;
#endif

enum class DB_Error
{
    NoError = 0,
    GenericError,
    DbBusyWhenClosing,
    DbNotOpen,
    EditorsStillOpened,
    FormatTooOld,
    FormatTooNew
};

//TODO: reorder functions
class MeetingSession : public QObject
{
    Q_OBJECT

private:
    static MeetingSession* session;
public:
    MeetingSession();
    ~MeetingSession();

    static MeetingSession* Get();

    inline ViewManager* getViewManager() { return viewManager.get(); }

    inline MetaDataManager* getMetaDataManager() { return metaDataMgr.get(); }

#ifdef ENABLE_BACKGROUND_MANAGER
    BackgroundManager *getBackgroundManager() const;
#endif

signals:
    //Shifts
    void shiftAdded(db_id shiftId);
    void shiftRemoved(db_id shiftId);
    void shiftNameChanged(db_id shiftId);

    //A job was added/removed/modified belonging to this shift
    void shiftJobsChanged(db_id shiftId, db_id jobId);

    //Rollingstock SYNC: wire them from models
    void rollingstockRemoved(db_id rsId);
    void rollingStockPlanChanged(db_id rsId);
    void rollingStockModified(db_id rsId);

    //Jobs
    void jobChanged(db_id jobId, db_id oldJobId); //Updated id/category/stops

    //Stations
    void stationNameChanged(db_id stationId);
    void stationPlanChanged(db_id stationId);
    void stationRemoved(db_id stationId);

    //Segments
    void segmentNameChanged(db_id segmentId);
    void segmentStationsChanged(db_id segmentId);
    void segmentRemoved(db_id segmentId);

    //Lines
    void lineNameChanged(db_id lineId);
    void lineSegmentsChanged(db_id lineId);
    void lineRemoved(db_id lineId);

//TODO: old methods, remove them
public:
    qreal getStationGraphPos(db_id lineId, db_id stId, int platf = 0);

    bool getPrevStop(db_id stopId, db_id &prevSt, db_id &lineId);
    bool getNextStop(db_id stopId, db_id &nextSt, db_id &lineId);
    Direction getStopDirection(db_id stopId, db_id stId);

private:
    std::unique_ptr<ViewManager> viewManager;

    std::unique_ptr<MetaDataManager> metaDataMgr;

#ifdef ENABLE_BACKGROUND_MANAGER
    std::unique_ptr<BackgroundManager> backgroundManager;
#endif

public:
    LineStorage *mLineStorage;
    JobStorage *mJobStorage;

//Settings TODO: remove
public:
    void loadSettings(const QString &settings_file);

    MRTPSettings settings;

    int    hourOffset;
    int    stationOffset;
    qreal  platformOffset;

    int    horizOffset;
    int    vertOffset;

    int jobLineWidth;

//Queries TODO: remove
public:
    database m_Db;

    query q_getPrevStop;
    query q_getNextStop;

    query q_getKmDirection;

//Categories:
public:
    QColor colorForCat(JobCategory cat);

//Savepoints TODO: seem unused
public:
    inline bool getDBDirty() { return !savepointList.isEmpty(); }

    bool setSavepoint(const QString& pointname = "RESTOREPOINT");
    bool releaseSavepoint(const QString& pointname = "RESTOREPOINT");
    bool revertToSavepoint(const QString& pointname = "RESTOREPOINT");
    bool releaseAllSavepoints();
    bool revertAll();

    QStringList savepointList;

//DB
public:
    DB_Error createNewDB(const QString &file);
    DB_Error openDB(const QString& str, bool ignoreVersion);
    DB_Error closeDB();

    bool checkImportRSTablesEmpty();
    bool clearImportRSTables();

    void prepareQueryes();
    void finalizeStatements();

    QString fileName; //TODO: re organize variables

//AppData
public:
    static void locateAppdata();
    static QString appDataPath;
};

#define Session MeetingSession::Get()

#define AppSettings Session->settings

#endif // MEETINGSESSION_H
