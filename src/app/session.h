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

#ifndef MEETINGSESSION_H
#define MEETINGSESSION_H

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QObject>

#include <QString>

#include <QColor>

#include <settings/appsettings.h>


class ViewManager;
class MetaDataManager;

#ifdef ENABLE_BACKGROUND_MANAGER
class BackgroundManager;
#endif

class QTranslator;

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

/*!
 * \brief The MeetingSession class
 *
 * This class is a singleton.
 * It stores all informations about current loaded session and settings
 */
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

    //Rollingstock
    void rollingstockRemoved(db_id rsId);
    void rollingStockPlanChanged(QSet<db_id> rsIds);
    void rollingStockModified(db_id rsId);

    //Jobs
    void jobAdded(db_id jobId);
    void jobChanged(db_id jobId, db_id oldJobId); //Updated id/category/stops
    void jobRemoved(db_id jobId);

    //Stations
    void stationNameChanged(db_id stationId);
    void stationJobsPlanChanged(const QSet<db_id>& stationIds);
    void stationTrackPlanChanged(const QSet<db_id>& stationIds);
    void stationRemoved(db_id stationId);

    //Segments
    void segmentAdded(db_id segmentId);
    void segmentNameChanged(db_id segmentId);
    void segmentStationsChanged(db_id segmentId);
    void segmentRemoved(db_id segmentId);

    //Lines
    void lineAdded(db_id lineId);
    void lineNameChanged(db_id lineId);
    void lineSegmentsChanged(db_id lineId);
    void lineRemoved(db_id lineId);

private:
    std::unique_ptr<ViewManager> viewManager;

    std::unique_ptr<MetaDataManager> metaDataMgr;

#ifdef ENABLE_BACKGROUND_MANAGER
    std::unique_ptr<BackgroundManager> backgroundManager;
#endif

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

//Database
public:
    database m_Db;

//Job Categories:
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

    QString fileName;

//AppData
public:
    static void locateAppdata();
    static QString appDataPath;

private:
    /*!
     * \brief sheetExportTranslator
     *
     * Custom translator for Sheet Export
     * When it's valid it should be used before application translations
     * When it's \c nullptr, application translations should be used
     * Special case: when it's \c nullptr and \a sheetExportLocale is \c QLocale(QLocale::English)
     * which is default language, then use hardcoded string literals and bypass any translation.
     *
     * \sa sheetExportLocale
     * \sa setSheetExportTranslator()
     */
    QTranslator *sheetExportTranslator;

    /*!
     * \brief sheetExportLocale
     *
     * Locale for Sheet Export
     * \sa sheetExportTranslator
     */
    QLocale sheetExportLocale;

    /*!
     * \brief originalAppLocale
     * Store original language in which application was loaded
     * When user changes Application Language in settings a restart
     * is required to apply it but settings reports already new language.
     * So use this variable to get original settings value before restart.
     */
    QLocale originalAppLocale;

    /*!
     * \brief setSheetExportTranslator
     * \param translator
     * \param loc
     *
     * Set translator for Sheet Export
     * \sa sheetExportTranslator
     */

public:
    /*!
     * \brief Embedded Locale
     *
     * This represents the QLocale of embedded strings (American English).
     * This is the default Application Language if no translations are loaded
     * If user choose this language no translations need to be loaded.
     */
    static const QLocale embeddedLocale;

    void setSheetExportTranslator(QTranslator *translator, const QLocale& loc);

    inline QTranslator *getSheetExportTranslator() const { return sheetExportTranslator; }
    inline QLocale getSheetExportLocale() const { return sheetExportLocale; }
    inline QLocale getAppLanguage() const { return originalAppLocale; }
};

#define Session MeetingSession::Get()

#define AppSettings Session->settings

#endif // MEETINGSESSION_H
