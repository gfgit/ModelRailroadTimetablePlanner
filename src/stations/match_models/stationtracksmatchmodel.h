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

#ifndef STATIONTRACKSMATCHMODEL_H
#define STATIONTRACKSMATCHMODEL_H

#include "utils/delegates/sql/isqlfkmatchmodel.h"
#include "utils/delegates/sql/imatchmodelfactory.h"

#include "utils/types.h"
#include "stations/station_utils.h"

#include <sqlite3pp/sqlite3pp.h>

#include <QFlags>

class StationTracksMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    StationTracksMatchModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString &text) override;
    virtual void refreshData() override;
    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

    // StationsMatchModel:
    void setFilter(db_id stationId);

private:
    struct TrackItem
    {
        db_id trackId;
        QString name;
        bool passenger;
        bool freight;
        QFlags<utils::StationTrackType> type;
    };
    static const int ItemCount = 30;
    TrackItem items[ItemCount];

    sqlite3pp::database &mDb;
    sqlite3pp::query q_getMatches;

    db_id m_stationId;
    QByteArray mQuery;
};

class StationTracksMatchFactory : public IMatchModelFactory
{
public:
    StationTracksMatchFactory(sqlite3pp::database &db, QObject *parent);

    virtual ISqlFKMatchModel *createModel() override;

    inline void setStationId(db_id stationId)
    {
        m_stationId = stationId;
    }

private:
    db_id m_stationId;
    sqlite3pp::database &mDb;
};

#endif // STATIONTRACKSMATCHMODEL_H
