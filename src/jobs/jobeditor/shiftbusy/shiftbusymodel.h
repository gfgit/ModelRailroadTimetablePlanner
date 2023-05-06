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

#ifndef SHIFTBUSYMODEL_H
#define SHIFTBUSYMODEL_H

#include <QAbstractTableModel>

#include <QVector>
#include <QTime>
#include "utils/types.h"

namespace sqlite3pp {
class database;
}

//TODO: move to shifts subdir
class ShiftBusyModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    enum Columns {
        JobCol = 0,
        Start,
        End,
        NCols
    };

    struct JobInfo
    {
        db_id jobId;
        QTime start;
        QTime end;
        JobCategory jobCat;
    };

    ShiftBusyModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void loadData(db_id shiftId, db_id jobId, const QTime& start, const QTime& end);

    inline bool hasConcurrentJobs() const { return m_data.size(); }

    inline QTime getStart() const { return m_start; }
    inline QTime getEnd() const { return m_end; }

    inline QString getShiftName() const { return m_shiftName; }
    QString getJobName() const;

private:
    sqlite3pp::database &mDb;
    QVector<JobInfo> m_data;

    db_id m_shiftId;
    db_id m_jobId;
    QTime m_start;
    QTime m_end;
    QString m_shiftName;
    JobCategory m_jobCat;
};

#endif // SHIFTBUSYMODEL_H
