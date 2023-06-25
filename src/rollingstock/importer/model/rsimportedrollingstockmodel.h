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

#ifndef RSIMPORTEDROLLINGSTOCKMODEL_H
#define RSIMPORTEDROLLINGSTOCKMODEL_H

#include "rollingstock/importer/intefaces/irsimportmodel.h"
#include <QVector>

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class RSImportedRollingstockModel : public IRsImportModel
{
    Q_OBJECT

public:
    enum
    {
        BatchSize = 100
    };

    enum Columns
    {
        Import = 0,
        Model,
        Number,
        Owner,
        NewNumber,
        NCols
    };

    struct RSItem
    {
        db_id importdRsId;
        db_id importedModelId;
        db_id importedModelMatchId;
        db_id importedOwnerId;
        int number;
        int new_number;
        QByteArray modelName;
        QByteArray ownerName;
        QByteArray ownerCustomName;
        bool import;
        RsType type;
    };

    RSImportedRollingstockModel(database &db, QObject *parent = nullptr);
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
    bool checkNewNumberIsValid(db_id importedRsId, db_id importedModelId,
                               db_id matchExistingModelId, RsType rsType, int number, int newNumber,
                               QString *errTextOut) override;

protected:
    virtual qint64 recalcTotalItemCount() override;

private:
    void fetchRow(int row);
    Q_INVOKABLE void internalFetch(int first, int sortCol, int valRow, const QVariant &val);
    void handleResult(const QVector<RSItem> &items, int firstRow);

private:
    QVector<RSItem> cache;
    int cacheFirstRow;
    int firstPendingRow;
};

#endif // RSIMPORTEDROLLINGSTOCKMODEL_H
