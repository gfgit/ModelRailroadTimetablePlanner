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

#ifndef RSMODELSSQLMODEL_H
#define RSMODELSSQLMODEL_H

#include "utils/delegates/sql/pageditemmodelhelper.h"

#include "utils/types.h"

namespace sqlite3pp {
class query;
}

struct RSModelsSQLModelItem
{
    db_id modelId;
    QString name;
    QString suffix;
    qint16 maxSpeedKmH;
    qint8 axes;
    RsType type;
    RsEngineSubType sub_type;
};


class RSModelsSQLModel : public IPagedItemModelImpl<RSModelsSQLModel, RSModelsSQLModelItem>
{
    Q_OBJECT

public:

    enum { BatchSize = 100 };

    enum Columns {
        Name = 0,
        Suffix,
        MaxSpeed,
        Axes,
        TypeCol,
        SubTypeCol,
        NCols
    };

    typedef RSModelsSQLModelItem RSModel;
    typedef IPagedItemModelImpl<RSModelsSQLModel, RSModelsSQLModelItem> BaseClass;

    RSModelsSQLModel(sqlite3pp::database &db, QObject *parent = nullptr);

    // QAbstractTableModel:

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& idx) const override;


    // IPagedItemModel:

    virtual void setSortingColumn(int col) override;

    //Filter
    std::pair<QString, FilterFlags> getFilterAtCol(int col) override;
    bool setFilterAtCol(int col, const QString& str) override;

    // RSModelsSQLModel:
    bool removeRSModel(db_id modelId, const QString &name);
    bool removeRSModelAt(int row);
    db_id addRSModel(int *outRow, db_id sourceModelId, const QString &suffix, QString *errOut);

    db_id getModelIdAtRow(int row) const;

    bool removeAllRSModels();

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    friend BaseClass;
    void buildQuery(sqlite3pp::query &q, int sortCol, int offset, bool fullData);
    Q_INVOKABLE void internalFetch(int first, int sortColumn, int valRow, const QVariant &val);

    bool setNameOrSuffix(RSModel &item, const QString &newName, bool suffix);
    bool setType(RSModel &item, RsType type, RsEngineSubType subType);

private:
    QString m_nameFilter;
    QString m_suffixFilter;
    QString m_speedFilter;
};

#endif // RSMODELSSQLMODEL_H
