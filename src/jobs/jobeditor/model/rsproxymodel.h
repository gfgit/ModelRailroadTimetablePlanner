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

#ifndef RSPROXYMODEL_H
#define RSPROXYMODEL_H

#include <QAbstractListModel>

#include <QTime>

#include "utils/types.h"

class RSCouplingInterface;

class RSProxyModel : public QAbstractListModel
{
    Q_OBJECT

public:
    RSProxyModel(RSCouplingInterface *mgr, RsOp o, RsType type, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    enum RSItemFlg
    {
        NoFlag              = 0,
        WrongStation        = 1,
        LastOperation       = 2,
        HasNextOperation    = 3,
        FirstUseOfRS        = 4,
        UnusedRS            = 5,
        ErrNotCoupledBefore = 6,
        ErrAlreadyCoupled   = 7
    };

    struct RsItem
    {
        db_id rsId;
        db_id jobId; // Can be next job or previous job
        QString rsName;
        int flag;
        QTime time; // Can be next or previous operation time
        JobCategory jobCat;
        RsEngineSubType engineType;
    };

    void loadData(const QList<RsItem> &items);

private:
    QList<RsItem> m_data;

    RSCouplingInterface *couplingMgr;
    RsOp op;
    RsType targetType;
};

#endif // RSPROXYMODEL_H
