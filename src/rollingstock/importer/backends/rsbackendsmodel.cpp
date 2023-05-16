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

#include "rsbackendsmodel.h"

RSImportBackendsModel::RSImportBackendsModel(QObject *parent) :
    QAbstractListModel(parent)
{

}

RSImportBackendsModel::~RSImportBackendsModel()
{
    qDeleteAll(backends);
    backends.clear();
}

int RSImportBackendsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : backends.size();
}

QVariant RSImportBackendsModel::data(const QModelIndex &idx, int role) const
{
    if(role != Qt::DisplayRole || idx.column() != 0)
        return QVariant();

    RSImportBackend *back = getBackend(idx.row());
    if(!back)
        return QVariant();

    return back->getBackendName();
}

void RSImportBackendsModel::addBackend(RSImportBackend *backend)
{
    const int row = backends.size();
    beginInsertRows(QModelIndex(), row, row);
    backends.append(backend);
    endInsertRows();
}

RSImportBackend *RSImportBackendsModel::getBackend(int idx) const
{
    if(idx < 0 || idx >= backends.size())
        return nullptr;
    return backends.at(idx);
}
