#ifndef STOPMODEL_H
#define STOPMODEL_H

#include <QAbstractListModel>
#include <QTime>

#include <QVector>
#include <QSet>

#include "stations/station_utils.h"

namespace sqlite3pp {
class database;
}

struct StopItem
{
    struct Gate
    {
        db_id gateConnId = 0;
        db_id gateId = 0;
        int gateTrackNum = -1;
        utils::Side stationTrackSide = utils::Side::NSides;
    };

    struct Segment
    {
        db_id segConnId = 0;
        db_id segmentId = 0;
        int inTrackNum  = -1;
        int outTrackNum = -1;
        bool reversed = false;
    };

    db_id stopId    = 0;
    db_id stationId = 0;
    db_id trackId   = 0;

    Gate fromGate;
    Gate toGate;
    Segment nextSegment;

    QTime arrival;
    QTime departure;

    int addHere = 0;

    StopType type = StopType::Normal;
};


/*!
 * \brief The StopModel class
 *
 * Item model to load and manage job stops
 *
 * \sa JobPathEditor
 * \sa StopEditor
 */
class StopModel : public QAbstractListModel
{
    Q_OBJECT
public:
    StopModel(sqlite3pp::database& db, QObject *parent = nullptr);

    // QAbstractListModel
    QVariant data(const QModelIndex &index, int role) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Options
    void setTimeCalcEnabled(bool value);
    void setAutoInsertTransits(bool value);
    void setAutoMoveUncoupleToNewLast(bool value);
    void setAutoUncoupleAtLast(bool value);

    // Loading
    bool loadJobStops(db_id jobId);
    void clearJob();

    // Saving
    bool isEdited() const;
    bool commitChanges();
    bool revertChanges();

    // Editing
    void addStop();
    void removeStop(const QModelIndex& idx);
    void removeLastIfEmpty();

    void uncoupleStillCoupledAtLastStop();
    void uncoupleStillCoupledAtStop(const StopItem &s);

    // Getters
    JobCategory getCategory() const;
    db_id getJobId() const;
    db_id getJobShiftId() const;
    db_id getNewShiftId() const;

    // Setters
    void setStopInfo(const QModelIndex& idx, StopItem newStop, StopItem::Segment prevSeg);

    bool setStopTypeRange(int firstRow, int lastRow, StopType type);

    // Stop Description
    QString getDescription(const StopItem &s) const;
    void setDescription(const QModelIndex &idx, const QString &descr);

    // Convinience
    int getStopRow(db_id stopId) const;

    bool isAddHere(const QModelIndex& idx);

    std::pair<QTime, QTime> getFirstLastTimes() const;

    const QSet<db_id> &getRsToUpdate() const;
    const QSet<db_id> &getStationsToUpdate() const;

    bool isRailwayElectrifiedAfterStop(db_id stopId) const;
    bool isRailwayElectrifiedAfterRow(int row) const;

    inline StopItem getItemAt(int row) const { return stops.at(row); }
    inline StopType getItemTypeAt(int row) const { return stops.at(row).type; }
    inline db_id getItemStationAt(int row) const { return stops.at(row).stationId; }


#ifdef ENABLE_AUTO_TIME_RECALC
    void rebaseTimesToSpeed(int firstIdx, QTime firstArr, QTime firstDep);
#endif

    bool trySelectTrackForStop(StopItem &item);

    bool trySetTrackConnections(StopItem &item, db_id trackId,
                               QString *outErr);

    bool trySelectNextSegment(StopItem &item, db_id segmentId, int suggestedOutGateTrk,
                              db_id nextStationId, db_id &out_gateId, db_id &out_suggestedTrackId);

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

    void onStationSegmentNameChanged();

private:
    void insertAddHere(int row, int type);
    db_id createStop(db_id jobId, const QTime &arr, const QTime &dep, StopType type);
    void deleteStop(db_id stopId);

    bool updateCurrentInGate(StopItem& curStop, const StopItem::Segment& prevSeg);
    bool updateStopTime(StopItem& item, int row, bool propagate, const QTime &oldArr, const QTime &oldDep);

    int calcTravelTime(db_id segmentId);
    int defaultStopTimeSec();

    void shiftStopsBy24hoursFrom(const QTime& startTime);

    friend class RSCouplingInterface;
    bool startInfoEditing();
    bool startStopsEditing();
    bool endStopsEditing();
    inline void markRsToUpdate(db_id rsId) { rsToUpdate.insert(rsId); }

private:
    //To simulate acceleration/braking we add 4 km to distance
    static constexpr double accelerationDistMeters = 4000.0;

    sqlite3pp::database& mDb;

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

    bool timeCalcEnabled;
    bool autoInsertTransits;
    bool autoMoveUncoupleToNewLast;
    bool autoUncoupleAtLast;
};

#endif // STOPMODEL_H
