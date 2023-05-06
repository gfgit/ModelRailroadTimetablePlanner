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

#include "stationimportwizard.h"

#include "utils/wizard/choosefilepage.h"
#include "pages/selectstationpage.h"

#include "utils/files/file_format_names.h"

#include "model/importstationmodel.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QDebug>

StationImportWizard::StationImportWizard(QWidget *parent) :
    QWizard(parent),
    mInMemory(false)
{
    mTempDB = new database;

    QStringList fileFormats;
    fileFormats << FileFormats::tr(FileFormats::tttFormat);
    fileFormats << FileFormats::tr(FileFormats::sqliteFormat);
    fileFormats << FileFormats::tr(FileFormats::allFiles);

    //Setup Pages
    QWizardPage *optionsPage = new QWizardPage;
    optionsPage->setTitle(tr("Import Stations"));
    optionsPage->setSubTitle(tr("Follow the steps to import stations"));

    ChooseFilePage *chooseFilePage = new ChooseFilePage;
    chooseFilePage->setFileDlgOptions(tr("Open Session"), fileFormats);
    connect(chooseFilePage, &ChooseFilePage::fileChosen, this, &StationImportWizard::onFileChosen);

    setPage(OptionsPageIdx, optionsPage);
    setPage(ChooseFileIdx, chooseFilePage);
    setPage(SelectStationIdx, new SelectStationPage(this));


    setWindowTitle(tr("Import Stations"));
    resize(700, 500);
}

StationImportWizard::~StationImportWizard()
{
    closeDatabase();

    if(mTempDB)
    {
        delete mTempDB;
        mTempDB = nullptr;
    }
}

void StationImportWizard::onFileChosen(const QString &fileName)
{
    createDatabase(false, fileName);
}

bool StationImportWizard::createDatabase(bool inMemory, const QString &fileName)
{
    closeDatabase();

    SelectStationPage *p = static_cast<SelectStationPage *>(page(SelectStationIdx));

    int flags = SQLITE_OPEN_READWRITE;
    if(inMemory)
        flags |= SQLITE_OPEN_MEMORY;

    int ret = mTempDB->connect(fileName.toUtf8(), flags);
    if(ret != SQLITE_OK)
        return false;

    mInMemory = inMemory;

    ImportStationModel *m = new ImportStationModel(*mTempDB, this);
    p->setupModel(m);
    return true;
}

bool StationImportWizard::closeDatabase()
{
    if(!mTempDB)
        return true;

    SelectStationPage *p = static_cast<SelectStationPage *>(page(SelectStationIdx));
    p->finalizeModel();

    return mTempDB->disconnect() == SQLITE_OK;
}

bool StationImportWizard::checkNames(db_id sourceStId, const QString &newName, QString &outShortName)
{
    if(!mTempDB || !mTempDB->db())
        return false;

    database &destDB = Session->m_Db;

    query q(*mTempDB, "SELECT name, short_name FROM stations WHERE id=?");
    q.bind(1, sourceStId);
    int ret = q.step();
    if(ret != SQLITE_ROW)
        return false;

    auto sourceSt = q.getRows();
    QString name = sourceSt.get<QString>(0);
    outShortName = sourceSt.get<QString>(1);
    q.finish();

    if(!newName.isEmpty())
        name = newName; //Override name

    query q_nameExists(destDB, "SELECT id FROM stations WHERE name=?1 OR short_name=?1 LIMIT 1");
    q_nameExists.bind(1, name);
    if(q_nameExists.step() == SQLITE_ROW)
        return false; //Name exists

    if(!outShortName.isEmpty())
    {
        q_nameExists.reset();
        q_nameExists.bind(1, outShortName);
        if(q_nameExists.step() == SQLITE_ROW)
            outShortName.clear(); //Name exists, remove short name
    }
    q_nameExists.finish();

    return true;
}

bool StationImportWizard::addStation(db_id sourceStId, const QString &fullName, const QString& shortName)
{
    if(!mTempDB || !mTempDB->db())
        return false;

    database &destDB = Session->m_Db;

    query q(*mTempDB, "SELECT type FROM stations WHERE id=?");
    q.bind(1, sourceStId);
    int ret = q.step();
    if(ret != SQLITE_ROW)
        return false;

    utils::StationType type = utils::StationType(q.getRows().get<int>(0));
    q.finish();

    transaction stTranaction(destDB);

    //Insert station
    //NOTE: do not import phone number as it's UNIQUE for our session so it may already exist for a different station
    //Set phone number to NULL, it can be set by the user afterwards if needed
    command cmd(destDB, "INSERT INTO stations(id,name,short_name,type,phone_number,svg_data)"
                        "VALUES(NULL,?,?,?,?,NULL)");
    cmd.bind(1, fullName);
    if(shortName.isEmpty())
        cmd.bind(2); //Bind NULL
    else
        cmd.bind(2, shortName);
    cmd.bind(3, int(type));
    cmd.bind(4); //Bind NULL phone number
    ret = cmd.execute();
    if(ret != SQLITE_OK)
        return false;

    db_id destStId = destDB.last_insert_rowid();

    QHash<db_id, db_id> gateMapping;
    QHash<db_id, db_id> trackMapping;

    //Copy tracks
    const QRgb whiteColor = qRgb(255, 255, 255);

    cmd.prepare("INSERT INTO station_tracks(id,station_id,pos,type,"
                "track_length_cm,platf_length_cm,freight_length_cm, max_axes,color_rgb,name)"
                "VALUES(NULL,?,?,?, ?,?,?, ?,?,?)");
    q.prepare("SELECT id,pos,type,"
              "track_length_cm,platf_length_cm,freight_length_cm, max_axes,color_rgb,name FROM station_tracks"
              " WHERE station_id=? ORDER BY pos ASC");
    q.bind(1, sourceStId);
    for(auto track : q)
    {
        db_id sourceTrackId = track.get<db_id>(0);
        int pos = track.get<int>(1);
        utils::StationTrackType trackType = utils::StationTrackType(track.get<int>(2));
        int trackLength_cm = track.get<int>(3);
        int platfLength_cm = track.get<int>(4);
        int freightLength_cm = track.get<int>(5);
        int maxAxes = track.get<int>(6);

        QRgb colorRGB = whiteColor;
        if(track.column_type(7) != SQLITE_NULL)
            colorRGB = QRgb(track.get<int>(7));

        QString trackName = track.get<QString>(8);

        cmd.bind(1, destStId);
        cmd.bind(2, pos);
        cmd.bind(3, int(trackType));
        cmd.bind(4, trackLength_cm);
        cmd.bind(5, platfLength_cm);
        cmd.bind(6, freightLength_cm);
        cmd.bind(7, maxAxes);

        if(colorRGB == whiteColor)
            cmd.bind(8); //Bind NULL
        else
            cmd.bind(8, int(colorRGB));

        cmd.bind(9, trackName);
        ret = cmd.execute();
        db_id destTrackId = destDB.last_insert_rowid();
        cmd.reset();

        if(ret == SQLITE_OK)
            trackMapping.insert(sourceTrackId, destTrackId);
    }

    //Copy gates
    cmd.prepare("INSERT INTO station_gates(id,station_id,out_track_count,type,def_in_platf_id,name,side)"
                "VALUES(NULL,?,?,?,?,?,?)");
    q.prepare("SELECT id,out_track_count,type,def_in_platf_id,name,side FROM station_gates"
              " WHERE station_id=? ORDER BY name ASC");
    q.bind(1, sourceStId);
    for(auto gate : q)
    {
        db_id sourceGateId = gate.get<db_id>(0);
        int outTrkCount = gate.get<int>(1);
        utils::GateType gateType = utils::GateType(gate.get<int>(2));
        db_id defPlatfId = gate.get<db_id>(3);
        QString gateName = gate.get<QString>(4);
        utils::Side side = utils::Side(gate.get<int>(5));

        //Map track ID if not NULL
        if(defPlatfId)
            defPlatfId = trackMapping.value(defPlatfId, 0);

        cmd.bind(1, destStId);
        cmd.bind(2, outTrkCount);
        cmd.bind(3, int(gateType));
        if(defPlatfId)
            cmd.bind(4, defPlatfId);
        else
            cmd.bind(4); //Bind NULL
        cmd.bind(5, gateName);
        cmd.bind(6, int(side));
        ret = cmd.execute();
        db_id destGateId = destDB.last_insert_rowid();
        cmd.reset();

        if(ret == SQLITE_OK)
            gateMapping.insert(sourceGateId, destGateId);
    }

    //Copy Gate connections
    cmd.prepare("INSERT INTO station_gate_connections(id,track_id,track_side,gate_id,gate_track)"
                "VALUES(NULL,?,?,?,?)");
    q.prepare("SELECT c.track_id, c.track_side, c.gate_id, c.gate_track"
              " FROM station_gate_connections c"
              " JOIN station_gates g ON g.id=c.gate_id"
              " WHERE g.station_id=?");
    q.bind(1, sourceStId);
    for(auto conn : q)
    {
        db_id trackId = conn.get<db_id>(0);
        utils::Side trackSide = utils::Side(conn.get<int>(1));
        db_id gateId = conn.get<db_id>(2);
        int gateTrk = conn.get<int>(3);

        //Map track and gate ID
        trackId = trackMapping.value(trackId, 0);
        gateId = gateMapping.value(gateId, 0);

        if(!trackId || !gateId)
            continue; //Error: could not map IDs, skip item

        cmd.bind(1, trackId);
        cmd.bind(2, int(trackSide));
        cmd.bind(3, gateId);
        cmd.bind(4, gateTrk);
        cmd.execute();
        cmd.reset();
    }

    //Copy SVG blob
    copySVGData(sourceStId, destStId);

    stTranaction.commit();
    return true;
}

struct SQLiteBlobDeleter
{
    static inline void cleanup(sqlite3_blob *pointer)
    {
        if(pointer)
            sqlite3_blob_close(pointer);
    }
};

bool StationImportWizard::copySVGData(db_id sourceStId, db_id destStId)
{
    database &destDB = Session->m_Db;

    //Copy SVG blob
    sqlite3_blob *ptr = nullptr;

    int ret = sqlite3_blob_open(mTempDB->db(), "main", "stations", "svg_data", sourceStId, 0, &ptr);
    if(ret != SQLITE_OK || !ptr)
    {
        qWarning() << "Station Import: cannot open source BLOB" << sourceStId << mTempDB->error_msg() << ret;
        return false;
    }

    QScopedPointer<sqlite3_blob, SQLiteBlobDeleter> mSourceBlob(ptr);
    ptr = nullptr;

    const int svgSize = sqlite3_blob_bytes(mSourceBlob.get());
    if(svgSize <= 0)
    {
        return true; //Nothing to copy
    }

    //Allocate SVG space
    command cmd(destDB, "UPDATE stations SET svg_data=? WHERE id=?");
    sqlite3_bind_zeroblob(cmd.stmt(), 1, svgSize);
    cmd.bind(2, destStId);
    ret = cmd.execute();
    if(ret != SQLITE_OK)
    {
        qWarning() << "Station Import: cannot allocate BLOB" << destStId << destDB.error_msg() << ret;
        return false;
    }

    ret = sqlite3_blob_open(destDB.db(), "main", "stations", "svg_data", destStId, 1, &ptr);
    if(ret != SQLITE_OK || !ptr)
    {
        qWarning() << "Station Import: cannot open destination BLOB" << destStId << destDB.error_msg() << ret;
        return false;
    }

    QScopedPointer<sqlite3_blob, SQLiteBlobDeleter> mDestBlob(ptr);
    ptr = nullptr;

    constexpr int SVG_BUF_SIZE = 8192;
    char svgBuf[SVG_BUF_SIZE] = {0};
    void *svgBufPtr = reinterpret_cast<void *>(svgBuf);
    int offset = 0;
    while(true)
    {
        int maxLen = SVG_BUF_SIZE;
        if(maxLen + offset >= svgSize)
            maxLen = svgSize - offset;

        if(maxLen <= 0)
            break;

        ret = sqlite3_blob_read(mSourceBlob.get(), svgBufPtr, maxLen, offset);
        if(ret != SQLITE_OK)
        {
            qWarning() << "Station Import: cannot read BLOB" << sourceStId << mTempDB->error_msg() << ret;
            break;
        }

        ret = sqlite3_blob_write(mDestBlob.get(), svgBufPtr, maxLen, offset);
        if(ret != SQLITE_OK || !mDestBlob)
        {
            qWarning() << "Station Import: cannot write BLOB" << destStId << destDB.error_msg() << ret;
            break;
        }

        offset += maxLen;
    }

    //Close blobs
    mSourceBlob.reset();
    mDestBlob.reset();

    return true;
}
