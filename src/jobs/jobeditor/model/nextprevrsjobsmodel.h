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

#ifndef NEXTPREVRSJOBSMODEL_H
#define NEXTPREVRSJOBSMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QTime>

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

/*!
 * \brief The NextPrevRSJobsModel class
 *
 * This model shows Next/Prev Jobs of currently set Job
 *
 * Next/Prev jobs are calculated based on coupled RS plans.
 * RS which do not have previous/next job are showed with "Depot" string
 */
class NextPrevRSJobsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Columns
    {
        JobIdCol = 0,
        RsNameCol,
        TimeCol,
        NCols
    };

    enum Mode
    {
        PrevJobs = 0,
        NextJobs
    };

    struct Item
    {
        db_id otherJobId = 0;
        db_id couplingId = 0;
        db_id rsId       = 0;
        db_id stopId     = 0;

        QString rsName;
        QTime opTime;

        JobCategory otherJobCat = JobCategory::NCategories;
    };

    NextPrevRSJobsModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &p = QModelIndex()) const override;
    int columnCount(const QModelIndex &p = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void refreshData();
    void clearData();

    db_id jobId() const;
    void setJobId(db_id newJobId);

    Mode mode() const;
    void setMode(Mode newMode);

    Item getItemAtRow(int row) const;

private:
    sqlite3pp::database &mDb;

    db_id m_jobId = 0;
    QVector<Item> m_data;

    Mode m_mode = PrevJobs;
};

#endif // NEXTPREVRSJOBSMODEL_H
