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

#include "rsproxymodel.h"

#include "rscouplinginterface.h"

#include "utils/jobcategorystrings.h"

#include <QBrush>

RSProxyModel::RSProxyModel(RSCouplingInterface *mgr, RsOp o, RsType type, QObject *parent) :
    QAbstractListModel(parent),
    couplingMgr(mgr),
    op(o),
    targetType(type)
{
}

int RSProxyModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

QVariant RSProxyModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.column() > 0 || idx.row() >= m_data.size())
        return QVariant();

    const RsItem &item = m_data.at(idx.row());

    switch (role)
    {
    case Qt::DisplayRole:
        return item.rsName;
    case Qt::CheckStateRole:
    {
        return couplingMgr->contains(item.rsId, op) ? Qt::Checked : Qt::Unchecked;
    }
    case Qt::BackgroundRole: // NOTE SYNC: RSCoupleDialog
    {
        if (item.flag == ErrNotCoupledBefore || item.flag == ErrAlreadyCoupled)
        {
            // Error: already coupled or already uncoupled or not coupled at all before this stop
            return QBrush(qRgb(255, 86, 255)); // Solid light magenta #FF56FF
        }
        if (item.flag == WrongStation)
        {
            // Error: RS is not in this station
            return QBrush(qRgb(255, 61, 67)); // Solid light red #FF3d43
        }
        if (targetType == RsType::Engine && item.engineType == RsEngineSubType::Electric
            && !couplingMgr->isRailwayElectrified())
        {
            // Warn Electric traction not possible
            return QBrush(qRgb(0, 0, 255), Qt::FDiagPattern); // Blue
        }
        if (item.flag == FirstUseOfRS)
        {
            return QBrush(qRgb(0, 255, 255)); // Cyan
        }
        if (item.flag == UnusedRS)
        {
            return QBrush(qRgb(0, 255, 0)); // Green
        }
        break;
    }
    case Qt::ToolTipRole:
    {
        if (item.flag == ErrNotCoupledBefore)
        {
            // Error
            return tr("Rollingstock <b>%1</b> cannot be uncoupled here because it wasn't coupled "
                      "to this job before this stop "
                      "or because it was already uncoupled before this stop.<br>"
                      "Please remove the tick")
              .arg(item.rsName);
        }
        if (item.flag == ErrAlreadyCoupled)
        {
            // Error
            if (item.jobId == couplingMgr->getJobId())
            {
                return tr("Rollingstock <b>%1</b> cannot be coupled here because it was already "
                          "coupled to this job before this stop<br>"
                          "Please remove the tick")
                  .arg(item.rsName);
            }
            else
            {
                return tr("Rollingstock <b>%1</b> cannot be coupled here because it was already "
                          "coupled before this stop<br>"
                          "to job <b>%2<b/><br>"
                          "Please remove the tick")
                  .arg(item.rsName, JobCategoryName::jobName(item.jobId, item.jobCat));
            }
        }
        if (item.flag == WrongStation)
        {
            // Error
            return tr("Rollingstock <b>%1</b> cannot be coupled here because it is not in this "
                      "station.<br>"
                      "Please remove the tick")
              .arg(item.rsName);
        }
        if (targetType == RsType::Engine && item.engineType == RsEngineSubType::Electric
            && !couplingMgr->isRailwayElectrified())
        {
            // Warn Electric traction not possible
            return tr("Engine <b>%1</b> is electric but the line is not electrified!")
              .arg(item.rsName);
        }
        if (item.flag == HasNextOperation)
        {
            return tr("Rollingstock <b>%1</b> is coupled in this station also by <b>%2</b> at "
                      "<b>%3</b>.")
              .arg(item.rsName, JobCategoryName::jobName(item.jobId, item.jobCat),
                   item.time.toString("HH:mm"));
        }
        if (item.flag == LastOperation)
        {
            return tr("Rollingstock <b>%1</b> was left in this station by <b>%2</b> at <b>%3</b>.")
              .arg(item.rsName, JobCategoryName::jobName(item.jobId, item.jobCat),
                   item.time.toString("HH:mm"));
        }
        if (item.flag == FirstUseOfRS)
        {
            if (op == RsOp::Coupled && couplingMgr->contains(item.rsId, RsOp::Coupled))
                return tr("This is the first use of this rollingstock <b>%1</b>").arg(item.rsName);
            return tr("This would be the first use of this rollingstock <b>%1</b>")
              .arg(item.rsName);
        }
        if (item.flag == UnusedRS)
        {
            return tr("Rollingstock <b>%1</b> is never used in this session. You can couple it for "
                      "the first time from any one station")
              .arg(item.rsName);
        }
        break;
    }
    }

    return QVariant();
}

bool RSProxyModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole || !idx.isValid() || idx.column() > 0
        || idx.row() >= m_data.size())
        return false;

    Qt::CheckState state = value.value<Qt::CheckState>();

    const RsItem &item   = m_data.at(idx.row());

    bool ret             = false;

    if (op == RsOp::Coupled) // Check traction type only if we are dealing with RsType::Engine
        ret = couplingMgr->coupleRS(item.rsId, item.rsName, state == Qt::Checked,
                                    targetType == RsType::Engine);
    else
        ret = couplingMgr->uncoupleRS(item.rsId, item.rsName, state == Qt::Checked);

    if (ret)
        emit dataChanged(idx, idx);

    return ret;
}

Qt::ItemFlags RSProxyModel::flags(const QModelIndex &index) const
{
    if (index.isValid())
        return Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsSelectable
               | Qt::ItemIsUserCheckable;
    return Qt::NoItemFlags;
}

void RSProxyModel::loadData(const QList<RSProxyModel::RsItem> &items)
{
    beginResetModel();
    m_data = items;
    endResetModel();
}
