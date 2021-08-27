#include "linestorage.h"

#include "app/session.h"

#include <QPen>

#include "lineobj.h"
#include <QGraphicsLineItem>

#include "jobs/jobstorage.h"

#include <QDebug>

#include <QFontMetrics>

/* LineStoragePrivate */

class LineStoragePrivate
{
public:
    typedef QHash<db_id, LineObj> LineLookup;
    typedef QHash<db_id, StationObj> StationLookup;

    inline LineStoragePrivate(database &db) :mDb(db) {}

    void clear();

    StationObj *getStation(db_id stationId);

    void addStation(LineObj &line, db_id stId);
    void removeStation(LineObj &line, db_id stId);

    void loadLine(LineObj &obj);
    LineLookup::iterator unloadLine(LineLookup::iterator it);

    void drawStations(LineObj &line);
    void drawJobs(db_id lineId);

public:
    database &mDb;
    LineLookup lines;
    StationLookup stations;
};

void LineStoragePrivate::clear()
{
    //Reset data
    for(auto it = lines.begin(); it != lines.end(); )
    {
        it = unloadLine(it);
    }
    lines.clear();
    lines.squeeze();
}

StationObj* LineStoragePrivate::getStation(db_id stationId)
{
    auto it = stations.find(stationId);
    if(it == stations.end())
    {
        //Load station
        query q(Session->m_Db, "SELECT id,platforms,depot_platf,platf_color FROM stations WHERE id=?");
        q.bind(1, stationId);
        if(q.step() != SQLITE_ROW)
            return nullptr;

        auto station = q.getRows();
        StationObj obj;
        obj.stId = station.get<db_id>(0);
        obj.platfs = station.get<qint8>(1);
        obj.depots = station.get<qint8>(2);

        //NULL is white (#FFFFFF) -> default value
        if(station.column_type(3) != SQLITE_NULL)
        {
            obj.platfRgb = QRgb(station.get<int>(3));
        }

        it = stations.insert(obj.stId, obj);
    }
    return &it.value();
}

void LineStoragePrivate::addStation(LineObj &line, db_id stId)
{
    StationObj *st = getStation(stId);
    if(!st)
        return;

    qreal x = Session->horizOffset;
    if(!line.stations.isEmpty())
    {
        const StationObj::Graph* prevGraph = line.stations.last();
        auto prevSt = stations.constFind(prevGraph->stationId);
        if(prevSt != stations.constEnd())
        {
            const int platfCount = prevSt->platfs + prevSt->depots;
            x = prevGraph->xPos + Session->platformOffset * platfCount + Session->stationOffset;
        }
    }

    StationObj::Graph *g = new StationObj::Graph;
    g->lineId = line.lineId;
    g->stationId = stId;
    g->xPos = x;

    st->lineGraphs.insert(line.lineId, g);
    line.stations.append(g);
}

void LineStoragePrivate::removeStation(LineObj &line, db_id stId)
{
    auto st = stations.find(stId);
    if(st == stations.end())
        return; //Error!

    auto it = st->lineGraphs.constFind(line.lineId);
    if(it == st->lineGraphs.constEnd())
        return; //Station isn't in this line

    const StationObj::Graph* g = it.value();
    qDeleteAll(g->platforms);

    st->lineGraphs.erase(it);
    if(st->lineGraphs.isEmpty())
    {
        //Unload station
        stations.erase(st);
    }

    for(int i = 0; i < line.stations.size(); i++)
    {
        if(line.stations.at(i) == g)
        {
            line.stations.removeAt(i);
            break;
        }
    }
    delete g;
}

void LineStoragePrivate::loadLine(LineObj &obj)
{
    query q_selectLineStations(mDb, "SELECT stationId FROM railways WHERE lineId=? ORDER BY pos_meters ASC");
    q_selectLineStations.bind(1, obj.lineId);
    for(auto r : q_selectLineStations)
    {
        db_id stId = r.get<db_id>(0);
        addStation(obj, stId);
    }
    q_selectLineStations.reset();

    Session->mJobStorage->loadLine(obj.lineId);
}

LineStoragePrivate::LineLookup::iterator LineStoragePrivate::unloadLine(LineLookup::iterator it)
{
    LineObj &line = it.value();

    Session->mJobStorage->unloadLine(line.lineId);

    for(StationObj::Graph *g : qAsConst(line.stations))
    {
        auto st = stations.find(g->stationId);
        Q_ASSERT_X(st != stations.end(), "LineStorage", "Line has ghost station");
        st->lineGraphs.remove(line.lineId);
        if(st->lineGraphs.isEmpty())
        {
            //Unload station
            stations.erase(st);
        }

        qDeleteAll(g->platforms);
        g->platforms.clear();

        for(StationObj::JobStop& s : g->stops)
        {
            delete s.line;
            delete s.text;
        }
        delete g;
    }
    line.stations.clear();
    //delete line.scene;
    //line.scene = nullptr;

    return lines.erase(it);
}

void LineStoragePrivate::drawStations(LineObj &line)
{
    const QRgb white = qRgb(255, 255, 255);
    qreal x = Session->horizOffset;
    db_id lastSt = 0;
    const int vertOffset = Session->vertOffset;
    const int stationOffset = Session->stationOffset;
    const double platfOffset = Session->platformOffset;
    const int lastY = vertOffset + Session->hourOffset * 24 + 10;

    const int width = AppSettings.getPlatformLineWidth();
    const QPen mainPlatfPen (AppSettings.getMainPlatfColor(),  width); //For main Platfs
    const QPen depotPlatfPen(AppSettings.getDepotPlatfColor(), width); //For depots

    const db_id lineId = line.lineId;
    //QGraphicsScene *scene = line.scene;

    line.stations.clear();

    query q_selectLineStations(mDb, "SELECT stationId FROM railways WHERE lineId=? ORDER BY pos_meters ASC");
    q_selectLineStations.bind(1, lineId);
    for(auto it = q_selectLineStations.begin(); it != q_selectLineStations.end(); ++it)
    {
        db_id stId = (*it).get<db_id>(0);
        lastSt = stId;

        StationObj *obj = getStation(stId);
        if(!obj)
        {
            //Error, do not assert here
            //It could be caused by a wrong insert into railways table
            //i.e. someone links a station to this line without telling LineStorage to sync
            continue;
        }

        auto iter = obj->lineGraphs.find(lineId);
        if(iter == obj->lineGraphs.end())
        {
            //Error
            continue;
        }

        StationObj::Graph* graph = iter.value();
        graph->xPos = x;

        line.stations.append(graph);

        //Clear stops, they will be re-inserted when we re-draw jobs
        for(StationObj::JobStop& s : graph->stops)
        {
            delete s.line;
            delete s.text;
        }
        graph->stops.clear();

        QVector<QGraphicsLineItem *>& vec = graph->platforms;
        const int oldSize = vec.count();
        const int size = obj->platfs + obj->depots;

        if(oldSize > size) //Delete extra
        {
            auto platf = vec.begin() + size;
            auto e = vec.end();
            qDeleteAll(platf, e);
            vec.erase(platf, e);
            vec.squeeze();
        }
        else
        {
            vec.reserve(size);
        }

        const int max = qMin(size, oldSize);
        for(int i = 0; i < max; i++)
        {
            auto l = vec.at(i);
            l->setLine(x, vertOffset,
                       x, lastY);
            x += platfOffset;
        }

        for(int i = oldSize; i < size; i++)
        {
//            auto l = scene->addLine(x, vertOffset,
//                                    x, lastY);
//            l->setZValue(-1); //Platform must be below jobs and labels
//            vec.append(l);
//            x += platfOffset;
        }


        QPen p = mainPlatfPen;
        if(obj->platfRgb != white) //If white (#FFFFFF) then use default color
        {
            p.setColor(obj->platfRgb);
        }

        int i = 0;
        for(; i < obj->platfs; i++) //Main Platf
        {
            auto l = vec.at(i);
            l->setPen(p);
        }
        for(; i < size; i++) //Depots
        {
            auto l = vec.at(i);
            l->setPen(depotPlatfPen);
        }

        x += stationOffset;
    }
    q_selectLineStations.reset();

    //Leave space for last station label
    query q(Session->m_Db, "SELECT name,short_name FROM stations WHERE id=?");
    q.bind(1, lastSt);
    auto station = q.getRows();
    QString lastName;
    if(station.column_bytes(1) == 0)
        lastName = station.get<QString>(0); //Fallback to full name
    else
        lastName = station.get<QString>(1);

    QFont f;
    f.setBold(true);
    f.setPointSize(15);
    int nameWidth = QFontMetrics(f).horizontalAdvance(lastName);

    //QRectF r = scene->itemsBoundingRect();
    //r.setRight(x - stationOffset + nameWidth);
    //r.adjust(0, 0, 0, 10);
    //r.setTopLeft(QPointF(0.0, 0.0));
    //scene->setSceneRect(r);
}

void LineStoragePrivate::drawJobs(db_id lineId)
{
    Session->mJobStorage->drawJobs(lineId);
}

/* LineStorage */

LineStorage::LineStorage(database &db, QObject *parent) :
    QObject(parent),
    mDb(db)
{
    impl = new LineStoragePrivate(db);
    connect(this, &LineStorage::stationModified, this, &LineStorage::onStationModified);
    connect(this, &LineStorage::stationColorChanged, this, &LineStorage::updateStationColor);
}

LineStorage::~LineStorage()
{
    delete impl;
    impl = nullptr;
}

void LineStorage::clear()
{
    impl->clear();
}

bool LineStorage::addLine(db_id *outLineId)
{
    if(!mDb.db())
        return false;

    sqlite3pp::command q_newLine(mDb, "INSERT INTO lines (id, name, max_speed, type) VALUES(NULL, '', 100, 1)");

    //TODO: First find possible empty row

    LineObj obj;
    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    q_newLine.execute();
    obj.lineId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_newLine.reset();

    if(obj.lineId == 0)
    {
        //Error
        qDebug() << mDb.error_msg();
        return false;
    }

    impl->lines.insert(obj.lineId, obj);

    emit lineAdded(obj.lineId);

    if(outLineId)
        *outLineId = obj.lineId;

    return true;
}

bool LineStorage::removeLine(db_id lineId)
{
    sqlite3pp::command q_removeLine(mDb, "DELETE FROM lines WHERE id=?");

    //BIG TODO: experimental foreign keys, instead of removing stations from line just block
    //Discuss this behaviour: maybe if there are no trains on this line we can remove it directly otherwise block and infor user

    //q_RemoveLineStations.bind(1, lineId);
    //q_RemoveLineStations.execute();
    //q_RemoveLineStations.reset();

    q_removeLine.bind(1, lineId);
    int ret = q_removeLine.execute();
    q_removeLine.reset();

    if(ret != SQLITE_OK)
    {
        qDebug() << Q_FUNC_INFO << "Error:" << ret << mDb.error_msg();
        int err = mDb.extended_error_code();
        qDebug() << "Extended error:" << err;
        if(err == SQLITE_CONSTRAINT_TRIGGER)
            qDebug() << "Line" << lineId << "IS IN USE";
        return false;
    }

    emit lineAboutToBeRemoved(lineId);

    auto it = impl->lines.find(lineId);
    if(it != impl->lines.end())
    {
        impl->unloadLine(it);
    }

    emit lineRemoved(lineId);

    return true;
}

bool LineStorage::addStation(db_id *outStationId)
{
    command q_newStation(mDb, "INSERT INTO stations(id,name, short_name,platforms,depot_platf, platf_color)"
                              " VALUES (NULL, '', NULL, 1, 0, NULL)");

    sqlite3_mutex *mutex = sqlite3_db_mutex(mDb.db());
    sqlite3_mutex_enter(mutex);
    int ret = q_newStation.execute();
    db_id stationId = mDb.last_insert_rowid();
    sqlite3_mutex_leave(mutex);
    q_newStation.reset();

    if(ret != SQLITE_OK || stationId == 0)
    {
        if(outStationId)
            *outStationId = 0;

        //Error
        qDebug() << mDb.error_msg();
        return false;
    }

    emit stationAdded(stationId);

    if(outStationId)
        *outStationId = stationId;

    return true;
}

bool LineStorage::removeStation(db_id stationId)
{
    command q_removeStation(mDb, "DELETE FROM stations WHERE id=?");

    q_removeStation.bind(1, stationId);
    int ret = q_removeStation.execute();
    q_removeStation.reset();

    if(ret != SQLITE_OK)
    {
        qDebug() << Q_FUNC_INFO << "Error:" << ret << mDb.error_msg();
        int err = mDb.extended_error_code();
        qDebug() << "Extended error:" << err;
        if(err == SQLITE_CONSTRAINT_TRIGGER)
        {
            qDebug() << "Station" << stationId << "IS IN USE";

            //emit modelError(tr(errorStationInUse).arg(iter.value().name));
        }
        return false;
    }

    emit stationRemoved(stationId);

    auto st = impl->stations.find(stationId);
    if(st == impl->stations.end())
        return true;

    //Delete station graph
    Q_ASSERT_X(st.value().lineGraphs.isEmpty(), "LineStorage", "Deleted station that was still in at least one line");
    impl->stations.erase(st);

    return true;
}

//You must manually redrawLine(lineId) after adding a station
void LineStorage::addStationToLine(db_id lineId, db_id stId)
{
    auto it = impl->lines.find(lineId);
    if(it == impl->lines.end())
        return;

    LineObj &obj = it.value();
    impl->addStation(obj, stId);
}

//You must manually redrawLine(lineId) after removing a station
void LineStorage::removeStationFromLine(db_id lineId, db_id stId)
{
    auto it = impl->lines.find(lineId);
    if(it == impl->lines.end())
        return;

    LineObj &obj = it.value();
    impl->removeStation(obj, stId);
}

bool LineStorage::releaseLine(db_id lineId)
{
    auto it = impl->lines.find(lineId);
    if(it == impl->lines.end())
        return false;
    LineObj &line = it.value();
    line.refCount--;
    if(line.refCount <= 0)
    {
        //Unload line FIXME: schedule deferred unsload, 5 seconds
        impl->unloadLine(it);
    }
    return true;
}

bool LineStorage::increfLine(db_id lineId)
{
    auto it = impl->lines.find(lineId);
    if(it == impl->lines.end())
    {
        //Load line, check if exists
        query q(mDb, "SELECT id FROM lines WHERE id=?");
        q.bind(1, lineId);
        if(q.step() != SQLITE_ROW)
            return false;

        LineObj obj;
        obj.lineId = lineId;

        it = impl->lines.insert(obj.lineId, obj);
        impl->loadLine(it.value());
        impl->drawStations(it.value());
        impl->drawJobs(it->lineId);
    }

    LineObj &line = it.value();
    line.refCount++;
    return true;
}

QGraphicsScene *LineStorage::sceneForLine(db_id lineId)
{
//    auto it = impl->lines.constFind(lineId);
//    if(it == impl->lines.constEnd())
//        return nullptr;
//    return it.value().scene;
}

void LineStorage::redrawAllLines()
{
    for(LineObj &obj : impl->lines)
    {
        impl->drawStations(obj);
        emit lineStationsModified(obj.lineId);
    }
}

void LineStorage::redrawLine(db_id lineId)
{
    auto it = impl->lines.find(lineId);
    if(it == impl->lines.end())
        return;
    impl->drawStations(it.value());
    impl->drawJobs(lineId);
    emit lineStationsModified(lineId);
}

void LineStorage::addJobStop(db_id stopId, db_id stId, db_id jobId, db_id lineId, const QString &label, qreal y1, qreal y2, int platf)
{
    //FIXME: check
    auto station = impl->stations.find(stId);
    if(station == impl->stations.end())
        return;
    station->addJobStop(stopId, jobId, lineId, label, y1, y2, platf);
}

//Removes all stop shadows in other lines
void LineStorage::removeJobStops(db_id jobId)
{
    for(StationObj &obj : impl->stations)
    {
        obj.removeJob(jobId);
    }
}

void LineStorage::updateStationColor(db_id stationId)
{
    if(!mDb.db())
        return;

    auto st = impl->stations.find(stationId);
    if(st == impl->stations.end())
        return;

    query q(mDb, "SELECT platf_color FROM stations WHERE id=?");
    q.bind(1, stationId);
    if(q.step() != SQLITE_ROW)
        return; //Error!
    QRgb color = QRgb(q.getRows().get<int>(0));
    st->setPlatfColor(color);
}

void LineStorage::onStationModified(db_id stationId)
{
    if(!mDb.db())
        return;

    auto st = impl->stations.find(stationId);
    if(st == impl->stations.end())
        return;

    query q(mDb, "SELECT platforms,depot_platf FROM stations WHERE id=?");
    q.bind(1, stationId);
    if(q.step() != SQLITE_ROW)
        return; //Error!
    auto r = q.getRows();
    st->platfs = r.get<int>(0);
    st->depots = r.get<int>(1);

    //Update all line this station is in and jobs that pass for this line.
    //Previously we did redrawAllLines() but that's a waste of time
    //And without pessimistic lock, instead of batching changes we get
    //'stationModified()' for every single change
    for(const auto &it : qAsConst(st->lineGraphs))
    {
        auto line = impl->lines.find(it->lineId);
        Q_ASSERT_X(line != impl->lines.end(), "LineStorage", "Station has a ghost line");

        impl->drawStations(line.value());
        impl->drawJobs(it->lineId);

        emit lineStationsModified(it->lineId);
    }
}
