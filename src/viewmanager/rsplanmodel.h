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

#ifndef RSPLANMODEL_H
#define RSPLANMODEL_H

#include <QAbstractTableModel>

#include <QVector>

#include <QTime>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

struct RsPlanItem
{
    db_id jobId;
    db_id stopId;
    db_id stationId;
    QTime arrival;
    QTime departure;
    QString stationName;
    JobCategory jobCat;
    RsOp op;
};

class RsPlanModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns
    {
        JobName = 0,
        Station,
        Arrival,
        Departure,
        Operation,
        NCols
    };

    RsPlanModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void loadPlan(db_id rsId);
    void clear();

    RsPlanItem getItem(int row);

private:
    sqlite3pp::database &mDb;

    QVector<RsPlanItem> m_data;
};

#endif // RSPLANMODEL_H
