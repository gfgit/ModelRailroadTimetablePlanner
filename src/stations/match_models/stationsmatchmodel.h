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

#ifndef STATIONSMATCHMODEL_H
#define STATIONSMATCHMODEL_H

#include "utils/delegates/sql/isqlfkmatchmodel.h"
#include "utils/delegates/sql/imatchmodelfactory.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class StationsMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT
public:
    StationsMatchModel(database &db, QObject *parent = nullptr);

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString &text) override;
    virtual void refreshData() override;
    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

    // StationsMatchModel:
    void setFilter(db_id exceptStId);

private:
    struct StationItem
    {
        db_id stationId;
        QString name;
        char padding[4];
    };
    StationItem items[MaxMatchItems];

    database &mDb;
    query q_getMatches;

    db_id m_exceptStId;

    QByteArray mQuery;
};

class StationMatchFactory : public IMatchModelFactory
{
    StationMatchFactory(sqlite3pp::database &db, QObject *parent);

    ISqlFKMatchModel *createModel() override;

    inline void setExceptStation(db_id stationId)
    {
        exceptStId = stationId;
    }

private:
    db_id exceptStId;
    sqlite3pp::database &mDb;
};

#endif // STATIONSMATCHMODEL_H
