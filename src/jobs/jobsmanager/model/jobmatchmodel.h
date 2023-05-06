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

#ifndef JOBMATCHMODEL_H
#define JOBMATCHMODEL_H

#include <QFont>

#include "utils/delegates/sql/isqlfkmatchmodel.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

//TODO: share common code with SearchResultModel
//TODO: allow filter byy job category

/*!
 * \brief Match model for Jobs
 *
 * Support filtering out 1 job (i.e. get 'others')
 * or get only jobs which stop in a specific station.
 * When specifying a station, you can also set a maximum time for arrival.
 * All jobs arriving later are excluded.
 * Otherwhise the arrival parameter is ignored if no station is set.
 */
class JobMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    enum Column
    {
        JobCategoryCol = 0,
        JobNumber,
        StopArrivalCol,
        NCols
    };

    enum DefaultId : bool
    {
        JobId = 0,
        StopId
    };

    JobMatchModel(database &db, QObject *parent = nullptr);

    // Basic functionality:
    int columnCount(const QModelIndex &p = QModelIndex()) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString& text) override;
    virtual void refreshData() override;
    QString getName(db_id jobId) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

    // JobMatchModel:
    void setFilter(db_id exceptJobId, db_id stopsInStationId, const QTime& maxStopArr);

    void setDefaultId(DefaultId defaultId);

private:
    struct JobItem
    {
        JobStopEntry stop;
        QTime stopArrival;
    };

    JobItem items[MaxMatchItems];

    database &mDb;
    query q_getMatches;

    db_id m_exceptJobId;
    db_id m_stopStationId;
    QTime m_maxStopArrival;
    DefaultId m_defaultId;

    QByteArray mQuery;

    QFont m_font;
};

#endif // JOBMATCHMODEL_H
