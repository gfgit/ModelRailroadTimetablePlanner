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

#ifndef RSIMPORTEDOWNERSMODEL_H
#define RSIMPORTEDOWNERSMODEL_H

#include "rollingstock/importer/intefaces/irsimportmodel.h"
#include <QVector>

#include "utils/types.h"
#include "utils/delegates/sql/IFKField.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class RSImportedOwnersModel : public IRsImportModel, public IFKField
{
    Q_OBJECT

public:
    enum
    {
        BatchSize = 100
    };

    enum Columns
    {
        SheetIdx = 0,
        Import,
        Name,
        CustomName,
        MatchExisting,
        NCols
    };

    struct OwnerItem
    {
        db_id importedOwnerId;
        db_id matchExistingId;
        QString name;
        QString customName;
        QString matchExistingName;
        int sheetIdx;
        bool import;
    };

    RSImportedOwnersModel(database &db, QObject *parent = nullptr);
    bool event(QEvent *e) override;

    // QAbstractTableModel

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &idx, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &idx) const override;

    // IPagedItemModel

    // Cached rows management
    virtual void clearCache() override;

    // Sorting TODO: enable multiple columns sort/filter with custom QHeaderView
    virtual void setSortingColumn(int col) override;

    // IRsImportModel:
    int countImported() override;

    // ICheckName:
    bool checkCustomNameValid(db_id importedOwnerId, const QString &originalName,
                              const QString &newCustomName, QString *errTextOut) override;

    // IFKField:
    bool getFieldData(int row, int col, db_id &ownerIdOut, QString &ownerNameOut) const override;
    bool validateData(int row, int col, db_id ownerId, const QString &ownerName) override;
    bool setFieldData(int row, int col, db_id ownerId, const QString &ownerName) override;

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortCol, int valRow, const QVariant &val);
    void handleResult(const QVector<OwnerItem> &items, int firstRow);

private:
    QVector<OwnerItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // RSIMPORTEDOWNERSMODEL_H
