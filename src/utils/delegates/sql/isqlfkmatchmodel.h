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

#ifndef ISQLFKMATCHMODEL_H
#define ISQLFKMATCHMODEL_H

#include <QAbstractTableModel>
#include "utils/types.h"

#define MaxMatchItems 10 //TODO: use constexpr

class ISqlFKMatchModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ISqlFKMatchModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    virtual void autoSuggest(const QString& text) = 0;
    virtual void refreshData();
    virtual void clearCache();

    virtual QString getName(db_id id) const;

    inline bool isEmptyRow(int row) const
    {
        //If it's last row and there isn't '...' or if it's just before '...'
        return hasEmptyRow && (row == EllipsesRow - 1 || (size < MaxMatchItems + 2 && row == size - 1));
    }

    inline bool isEllipsesRow(int row) const
    {
        //If there isn't 'Empty' row, ellipses come 1 row before EllipsesRow
        return row == (EllipsesRow - (hasEmptyRow ? 0 : 1));
    }

    virtual db_id getIdAtRow(int row) const = 0;
    virtual QString getNameAtRow(int row) const = 0;

    void setHasEmptyRow(bool value);

signals:
    void resultsReady(bool forceFirst);

protected:
    static QVariant boldFont();
    const static QString ellipsesString;

protected:
    bool hasEmptyRow;

public:
    enum {
        EllipsesRow = MaxMatchItems + 1 //(10 items + Empty + ...)
        //Items: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        //Empty: 10
        //Ellipses: 11
    };
    int size;
};

#endif // ISQLFKMATCHMODEL_H
