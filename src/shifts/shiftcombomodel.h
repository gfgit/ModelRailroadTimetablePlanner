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

#ifndef SHIFTCOMBOMODEL_H
#define SHIFTCOMBOMODEL_H

#include "utils/delegates/sql/isqlfkmatchmodel.h"
#include <QVector>

#include "shifts/shiftitem.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class ShiftComboModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    enum Columns
    {
        ShiftName = 0,
        NCols
    };

    ShiftComboModel(database &db, QObject *parent = nullptr);

    // Basic functionality:
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    bool event(QEvent *e) override;

    // ISqlFKMatchModel:
    void autoSuggest(const QString& text) override;
    void refreshData() override;
    void clearCache() override;
    QString getName(db_id shiftId) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

private:
    int recalcRowCount();
    void fetchRow();
    Q_INVOKABLE void internalFetch();

private:
    struct ShiftItem
    {
        db_id shiftId;
        QString name;
        char padding[4];
    };
    ShiftItem items[MaxMatchItems];

    database &mDb;

    QString mQuery;

    int totalCount;
    bool isFetching;
};

#endif // SHIFTCOMBOMODEL_H
