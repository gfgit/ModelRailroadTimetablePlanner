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

#ifndef STATIONPLANMODEL_H
#define STATIONPLANMODEL_H

#include <QAbstractTableModel>

#include <QTime>

#include <sqlite3pp/sqlite3pp.h>

#include "utils/types.h"

struct StPlanItem
{
    db_id stopId;
    db_id jobId;
    QTime arrival;
    QTime departure;
    QString platform;

    JobCategory cat;

    enum class ItemType : qint8
    {
        Normal = 0,
        Departure,
        Transit
    };
    ItemType type;

    bool reversesDirection;
};

class StationPlanModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns
    {
        Arrival = 0,
        Departure,
        Platform,
        Job,
        Notes,
        NCols
    };
    StationPlanModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void clear();

    void loadPlan(db_id stId);

    inline StPlanItem getItemAt(int row) const
    {
        if (row < m_data.size())
        {
            return m_data.at(row);
        }
        return StPlanItem();
    }

private:
    QList<StPlanItem> m_data;

    sqlite3pp::database &mDb;
    sqlite3pp::query q_countPlanItems;
    sqlite3pp::query q_selectPlan;
};

#endif // STATIONPLANMODEL_H
