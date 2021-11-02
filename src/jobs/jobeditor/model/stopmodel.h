#ifndef STOPMODEL_H
#define STOPMODEL_H

#include <QAbstractListModel>
#include <QTime>

#include <QVector>
#include <QSet>

#include "utils/model_roles.h"

#include "utils/types.h"

#include "sqlite3pp/sqlite3pp.h"
using namespace sqlite3pp;

class StopItem
{
public:
    struct Gate
    {
        db_id gateConnId = 0;
        db_id gateId = 0;
        int trackNum = 0;
    };

    struct Segment
    {
        db_id segConnId = 0;
        db_id segmentId = 0;
        int outTrackNum = 0;
    };

    db_id stopId       = 0;
    db_id stationId    = 0;

    db_id trackId      = 0;

    Gate fromGate;
    Gate toGate;
    Segment nextSegment;


    db_id segment      = 0;
    db_id nextSegment_  = 0;

    db_id curLine      = 0;
    db_id nextLine     = 0;

    int addHere           = 0;
    int platform          = 0;

    QTime arrival;
    QTime departure;

    StopType type         = Normal;
};

//BIG TODO: when changing arrival to a station where a RS is (un)coupled, the station is marked for update but not the RS
//          if a stop is removed, couplings get removed too but RS are not marked for update, also if Job is removed, needs also RsErrorCheck
class StopModel : public QAbstractListModel
{
    Q_OBJECT
public:
    StopModel(sqlite3pp::database& db, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    typedef enum
    {
        NoError = 0,
        GenericError = 1,
        ErrorInvalidIndex,
        ErrorInvalidArgument,
        ErrorFirstLastTransit,
        ErrorTransitWithCouplings
    } ErrorCodes;

    void setTimeCalcEnabled(bool value);
    void setAutoInsertTransits(bool value);
    void setAutoMoveUncoupleToNewLast(bool value);

    void prepareQueries();
    void finalizeQueries();

    void loadJobStops(db_id jobId);
    void clearJob();

    void addStop();
    void insertStopBefore(const QModelIndex& idx);
    void removeStop(const QModelIndex& idx);
    void removeLastIfEmpty();

    bool isAddHere(const QModelIndex& idx);

    bool updatePrevSegment(int row, StopItem& prevStop, StopItem &curStop, db_id segmentId);
    void setStopInfo(const QModelIndex& idx, const StopItem& newItem, const StopItem::Segment& prevSeg);

    void setArrival(const QModelIndex &idx, const QTime &time, bool setDepTime);
    void setDeparture(const QModelIndex &index, QTime time, bool propagate);
    int setStopType(const QModelIndex &idx, StopType type);
    int setStopTypeRange(int firstRow, int lastRow, StopType type);
    void setLine(const QModelIndex &idx, db_id lineId);
    bool lineHasSt(db_id lineId, db_id stId);
    void setStation(const QPersistentModelIndex &idx, db_id stId);
    void setPlatform(const QModelIndex &idx, int platf);

    QString getDescription(const StopItem &s) const;
    void setDescription(const QModelIndex &idx, const QString &descr);

    int calcTimeBetweenStInSecs(db_id stA, db_id stB, db_id lineId);
    int defaultStopTimeSec();

    std::pair<QTime, QTime> getFirstLastTimes() const;

    bool isEdited() const;
    bool commitChanges();
    bool revertChanges();

    JobCategory getCategory() const;
    db_id getJobId() const;
    db_id getJobShiftId() const;
    db_id getNewShiftId() const;

    int getStopRow(db_id stopId) const;

    const QSet<db_id> &getRsToUpdate() const;
    const QSet<db_id> &getStationsToUpdate() const;
    inline void markRsToUpdate(db_id rsId) { rsToUpdate.insert(rsId); }

    LineType getLineTypeAfterStop(db_id stopId) const;

    inline StopItem getItemAt(int row) const { return stops.at(row); }

    void uncoupleStillCoupledAtLastStop();
    void uncoupleStillCoupledAtStop(const StopItem &s);

    bool getStationPlatfCount(db_id stationId, int &platfCount, int &depotCount);

#ifdef ENABLE_AUTO_TIME_RECALC
    void rebaseTimesToSpeed(int firstIdx, QTime firstArr, QTime firstDep);
#endif


    void setAutoUncoupleAtLast(bool value);

signals:
    void edited(bool val);

    void categoryChanged(int newCat);
    void jobIdChanged(db_id jobId);
    void jobShiftChanged(db_id shiftId);
    void errorSetShiftWithoutStops(); //TODO: find better way to show errors

public slots:
    void setCategory(int value);
    bool setNewJobId(db_id jobId);
    void setNewShiftId(db_id shiftId);

private slots:
    void reloadSettings();

    void onExternalShiftChange(db_id shiftId, db_id jobId);
    void onShiftNameChanged(db_id shiftId);

    void onStationLineNameChanged();

private:
    //To simulate acceleration/braking we add 4 km to distance
    static constexpr double accelerationDistMeters = 4000.0;

    QVector<StopItem> stops;

    QSet<db_id> rsToUpdate;
    QSet<db_id> stationsToUpdate;

    db_id mJobId;
    db_id mNewJobId;

    db_id jobShiftId;
    db_id newShiftId;

    JobCategory category;
    JobCategory oldCategory;

    enum EditState
    {
        NotEditing = 0,
        InfoEditing = 1,
        StopsEditing = 2
    };

    EditState editState;

    database& mDb;

    //TODO: do not store queries, prepare them when needed
    query q_segPos;
    query q_getRwNode;
    query q_lineHasSt;
    query q_getCoupled;

    command q_setArrival;
    command q_setDeparture;

    command q_setSegPos;
    command q_setSegLine;
    command q_setStopSeg;
    command q_setNextSeg;
    command q_setStopSt;
    command q_removeSeg;
    command q_setPlatform;

    bool timeCalcEnabled;
    bool autoInsertTransits;
    bool autoMoveUncoupleToNewLast;
    bool autoUncoupleAtLast;

private:
    void insertAddHere(int row, int type);
    db_id createStop(db_id jobId, db_id segId, const QTime &time, int transit = 0);
    db_id createSegment(db_id jobId, int num);
    db_id createSegmentAfter(db_id jobId, db_id prevSeg);
    void setStopSeg(StopItem &s, db_id segId);
    void setNextSeg(StopItem &s, db_id nextSeg);
    void destroySegment(db_id segId, db_id jobId);
    void resetStopsLine(int idx, StopItem &s);
    void propagateLineChange(int idx, StopItem &s, db_id lineId);
    void deleteStop(db_id stopId);
    int propageteTimeOffset(int row, const int msecOffset);
    void insertTransitsBefore(const QPersistentModelIndex &stop);
    void setStation_internal(StopItem &item, db_id stId, db_id nodeId);
    void shiftStopsBy24hoursFrom(const QTime& startTime);

    friend class RSCouplingInterface;
    bool startInfoEditing();
    bool startStopsEditing();
    bool endStopsEditing();
};

#endif // STOPMODEL_H
