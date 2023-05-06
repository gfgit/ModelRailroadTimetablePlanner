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

#ifndef ROLLINGSTOCKMATCHMODEL_H
#define ROLLINGSTOCKMATCHMODEL_H

#include "utils/delegates/sql/isqlfkmatchmodel.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include <QRegularExpression>

class RollingstockMatchModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    enum Columns
    {
        ModelCol = 0,
        NumberCol,
        SuffixCol,
        OwnerCol,
        NCols
    };
    RollingstockMatchModel(database &db, QObject *parent = nullptr);

    // QAbstractTableModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel
    void autoSuggest(const QString& text) override;
    virtual void refreshData() override;
    QString getName(db_id id) const override;

    db_id getIdAtRow(int row) const override;
    QString getNameAtRow(int row) const override;

private:
    struct RSItem
    {
        db_id rsId;
        QString modelName;
        QString modelSuffix;
        QString ownerName;
        int number;
        RsType type;
    };
    RSItem items[MaxMatchItems];

    database &mDb;
    query q_getMatches;

    QRegularExpression regExp;
    QByteArray model,owner,number;
};

#endif // ROLLINGSTOCKMATCHMODEL_H
