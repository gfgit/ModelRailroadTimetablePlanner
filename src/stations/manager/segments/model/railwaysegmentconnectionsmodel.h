#ifndef RAILWAYSEGMENTCONNECTIONSMODEL_H
#define RAILWAYSEGMENTCONNECTIONSMODEL_H

#include <QAbstractTableModel>

#include <QVector>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class RailwaySegmentConnectionsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    typedef enum {
        FromGateTrackCol = 0,
        ToGateTrackCol,
        NCols
    } Columns;

    typedef enum {
        NoChange = 0,
        AddedButNotComplete,
        ToAdd,
        ToRemove,
        Edited
    } ItemState;

    typedef struct RailwayTrack_
    {
        db_id connId;
        int fromTrack;
        int toTrack;
        ItemState state;
    } RailwayTrack;

    static constexpr const int InvalidTrack = -1;

    RailwaySegmentConnectionsModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& idx) const override;

    void setSegment(db_id segmentId, db_id fromGateId, db_id toGateId, bool reversed);
    void resetData();
    void clear();

    inline int getActualCount() const { return actualCount; }
    inline void setReadOnly(bool val) { readOnly = val; }
    inline bool isReadOnly() const  { return readOnly; }

    inline int getFromGateTrackCount() const { return m_fromGateTrackCount; }
    inline int getToGateTrackCount() const { return m_toGateTrackCount; }

    void createDefaultConnections();

    void removeAtRow(int row);
    void addNewConnection(int *outRow);

    void applyChanges(db_id overrideSegmentId);

signals:
    void modelError(const QString& msg);

private:
    int insertOrReplace(const RailwayTrack& newTrack);

private:
    sqlite3pp::database &mDb;

    QVector<RailwayTrack> items;

    db_id m_segmentId;
    db_id m_fromGateId;
    db_id m_toGateId;
    int m_fromGateTrackCount;
    int m_toGateTrackCount;
    int actualCount;
    int incompleteRowAdded;
    bool m_reversed;
    bool readOnly;
};

#endif // RAILWAYSEGMENTCONNECTIONSMODEL_H
