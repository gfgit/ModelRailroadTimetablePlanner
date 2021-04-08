#ifndef RAILWAYSEGMENTMATCHMODEL_H
#define RAILWAYSEGMENTMATCHMODEL_H

#include "utils/sqldelegate/isqlfkmatchmodel.h"

#include "utils/types.h"
#include "stations/station_utils.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QFlags>

class RailwaySegmentMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    explicit RailwaySegmentMatchModel(sqlite3pp::database &db,
                                      QObject *parent = nullptr);

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString& text) override;
    virtual void refreshData() override;
    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

    // RailwaySegmentMatchModel:
    void setFilter(db_id fromStationId, db_id toStationId, db_id excludeSegmentId);

    bool isReversed(db_id segId) const;

private:
    struct SegmentItem
    {
        db_id segmentId;
        db_id toStationId;
        QString segmentName;
        QFlags<utils::RailwaySegmentType> type;
        bool reversed;
    };
    SegmentItem items[MaxMatchItems];

    sqlite3pp::database &mDb;
    sqlite3pp::query q_getMatches;

    db_id m_fromStationId;
    db_id m_toStationId;
    db_id m_excludeSegmentId;
    QByteArray mQuery;
};

#endif // RAILWAYSEGMENTMATCHMODEL_H
