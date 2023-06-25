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

#ifndef STATIONGATESMATCHMODEL_H
#define STATIONGATESMATCHMODEL_H

#include "utils/delegates/sql/isqlfkmatchmodel.h"
#include "utils/delegates/sql/imatchmodelfactory.h"

#include "utils/types.h"
#include "stations/station_utils.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QFlags>

class StationGatesMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    StationGatesMatchModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString &text) override;
    virtual void refreshData() override;
    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

    // StationsMatchModel:
    void setFilter(db_id stationId, bool markConnectedGates, db_id excludeSegmentId,
                   bool showOnlySegments = false);

    int getOutTrackCount(db_id gateId) const;
    utils::Side getGateSide(db_id gateId) const;

    db_id getSegmentIdAtRow(int row) const;
    db_id isSegmentReversedAtRow(int row) const;

    int getGateTrackCount(db_id gateId) const;

private:
    struct GateItem
    {
        db_id gateId;
        db_id segmentId;
        QChar gateLetter;
        QString segmentName;
        int outTrackCount;
        QFlags<utils::GateType> type;
        utils::Side side;
        bool segmentReversed;
    };
    static const int ItemCount = 30;
    GateItem items[ItemCount];

    sqlite3pp::database &mDb;
    sqlite3pp::query q_getMatches;

    db_id m_stationId;
    db_id m_excludeSegmentId;
    bool m_markConnectedGates;
    bool m_showOnlySegments;
    QByteArray mQuery;
};

class StationGatesMatchFactory : public IMatchModelFactory
{
public:
    StationGatesMatchFactory(sqlite3pp::database &db, QObject *parent);

    virtual ISqlFKMatchModel *createModel() override;

    inline void setStationId(db_id stationId)
    {
        m_stationId = stationId;
    }
    inline void setMarkConnectedGates(bool val, db_id excludeSegId)
    {
        markConnectedGates = val;
        m_excludeSegmentId = excludeSegId;
    }

private:
    db_id m_stationId;
    db_id m_excludeSegmentId;
    bool markConnectedGates;
    sqlite3pp::database &mDb;
};

#endif // STATIONGATESMATCHMODEL_H
