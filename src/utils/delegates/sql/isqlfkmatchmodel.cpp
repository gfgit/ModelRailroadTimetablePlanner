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

#include "isqlfkmatchmodel.h"

#include <QFont>

const QString ISqlFKMatchModel::ellipsesString = QStringLiteral("...");

ISqlFKMatchModel::ISqlFKMatchModel(QObject *parent) :
    QAbstractTableModel(parent),
    hasEmptyRow(true),
    size(0)
{
}

int ISqlFKMatchModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : size;
}

int ISqlFKMatchModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

void ISqlFKMatchModel::refreshData()
{
}

void ISqlFKMatchModel::clearCache()
{
}

QString ISqlFKMatchModel::getName(db_id /*id*/) const
{
    return QString();
}

QFont initBoldFont()
{
    QFont f;
    f.setBold(true);
    f.setItalic(true);
    return f;
}

// static
QVariant ISqlFKMatchModel::boldFont()
{
    static QFont emptyRowF = initBoldFont();
    return emptyRowF;
}

void ISqlFKMatchModel::setHasEmptyRow(bool value)
{
    hasEmptyRow = value;
}
