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

#include "pagesizemodel.h"

#include <QPageSize>

PageSizeModel::PageSizeModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

int PageSizeModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : QPageSize::NPageSize;
}

QVariant PageSizeModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || role != Qt::DisplayRole)
        return QVariant();

    QPageSize::PageSizeId pageId = QPageSize::PageSizeId(idx.row());
    return QPageSize::name(pageId);
}
