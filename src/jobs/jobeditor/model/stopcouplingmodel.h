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

#ifndef STOPCOUPLINGMODEL_H
#define STOPCOUPLINGMODEL_H

#include "rslistondemandmodel.h"


class StopCouplingModel : public RSListOnDemandModel
{
    Q_OBJECT

public:
    StopCouplingModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // StopCouplingModel
    void setStop(db_id stopId, RsOp op);

protected:
    // IPagedItemModel
    // Cached rows management
    virtual qint64 recalcTotalItemCount() override;

private:
    virtual void internalFetch(int first, int sortColumn, int valRow, const QVariant &val) override;

private:
    db_id m_stopId;
    RsOp m_operation;
};

#endif // STOPCOUPLINGMODEL_H
