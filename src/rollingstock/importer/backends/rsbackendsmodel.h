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

#ifndef OPTIONSMODEL_H
#define OPTIONSMODEL_H

#include <QAbstractListModel>

#include <QVector>
#include "rsimportbackend.h"

class RSImportBackendsModel : public QAbstractListModel
{
public:
    RSImportBackendsModel(QObject *parent);
    ~RSImportBackendsModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const;

    void addBackend(RSImportBackend *backend);

    RSImportBackend *getBackend(int idx) const;

private:
    QVector<RSImportBackend *> backends;
};

#endif // OPTIONSMODEL_H
