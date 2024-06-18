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

#ifndef JOBPASSINGSMODEL_H
#define JOBPASSINGSMODEL_H

#include <QAbstractTableModel>

#include <QList>

#include <QTime>

#include "utils/types.h"

class JobPassingsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns
    {
        JobNameCol = 0,
        ArrivalCol,
        DepartureCol,
        PlatformCol,
        NCols
    };

    struct Entry
    {
        db_id jobId;
        QTime arrival;
        QTime departure;
        QString platform;
        JobCategory category;
    };

    explicit JobPassingsModel(QObject *parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void setJobs(const QList<Entry> &vec);

private:
    QList<Entry> m_data;
};

#endif // JOBPASSINGSMODEL_H
