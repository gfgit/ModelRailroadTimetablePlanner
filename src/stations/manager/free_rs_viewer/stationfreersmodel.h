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

#ifndef STATIONFREERSMODEL_H
#define STATIONFREERSMODEL_H

#include <QAbstractTableModel>

#include <QVector>

#include <QTime>

#include <sqlite3pp/sqlite3pp.h>

#include "utils/types.h"

//TODO: on-demand load and let SQL do the sorting
class StationFreeRSModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns {
        RSNameCol = 0,
        FreeFromTimeCol,
        FreeUpToTimeCol,
        FromJobCol,
        ToJobCol,
        NCols
    };

    struct Item
    {
        db_id rsId = 0;
        QTime from; //Time at which RS is uncoupled (from now it's free)
        QTime to; //Time at which is coupled (not free anymore)
        QString name;
        db_id fromJob = 0;
        db_id fromStopId = 0;
        db_id toJob = 0;
        db_id toStopId = 0;
        JobCategory fromJobCat;
        JobCategory toJobCat;
    };

    StationFreeRSModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    const Item *getItemAt(int row) const;

    void setStation(db_id stId);
    void setTime(QTime time);

    enum ErrorCodes
    {
        NoError = 0,
        NoOperationFound,
        DBError
    };

    ErrorCodes getNextOpTime(QTime &time);
    ErrorCodes getPrevOpTime(QTime& time);

    QTime getTime() const;
    db_id getStationId() const;

    QString getStationName() const;

    bool sortByColumn(int col);

    int getSortCol() const;

public slots:
    void reloadData();

private:
    db_id m_stationId;
    QTime m_time;
    int sortCol;

    QVector<Item> m_data;

    sqlite3pp::database& mDb;
};

#endif // STATIONFREERSMODEL_H
