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

#ifndef RSLISTONDEMANDMODEL_H
#define RSLISTONDEMANDMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"

#include <QVector>

struct RSListOnDemandModelItem
{
    db_id rsId;
    QString name;
    RsType type;
};

class RSListOnDemandModel : public IPagedItemModelImpl<RSListOnDemandModel, RSListOnDemandModelItem>
{
    Q_OBJECT

public:
    enum { BatchSize = 50 };

    enum Columns {
        Name = 0,
        NCols
    };

    typedef RSListOnDemandModelItem RSItem;
    typedef IPagedItemModelImpl<RSListOnDemandModel, RSListOnDemandModelItem> BaseClass;

    RSListOnDemandModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

private:
    friend BaseClass;
    virtual void internalFetch(int first, int sortColumn, int valRow, const QVariant &val) = 0;
};

#endif // RSLISTONDEMANDMODEL_H
