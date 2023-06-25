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

#ifndef RSOWNERSSQLMODEL_H
#define RSOWNERSSQLMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"

struct RSOwnersSQLModelItem
{
    db_id ownerId;
    QString name;
};

class RSOwnersSQLModel : public IPagedItemModelImpl<RSOwnersSQLModel, RSOwnersSQLModelItem>
{
    Q_OBJECT

public:
    enum
    {
        BatchSize = 100
    };

    enum Columns
    {
        Name = 0,
        NCols
    };

    typedef RSOwnersSQLModelItem RSOwner;
    typedef IPagedItemModelImpl<RSOwnersSQLModel, RSOwnersSQLModelItem> BaseClass;

    RSOwnersSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &idx) const override;

    // IPagedItemModel

    // Filter
    std::pair<QString, FilterFlags> getFilterAtCol(int col) override;
    bool setFilterAtCol(int col, const QString &str) override;

    // RSOwnersSQLModel

    bool removeRSOwner(db_id ownerId, const QString &name);
    bool removeRSOwnerAt(int row);
    db_id addRSOwner(int *outRow);

    bool removeAllRSOwners();

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    friend BaseClass;
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);

private:
    QString m_ownerFilter;
};

#endif // RSOWNERSSQLMODEL_H
