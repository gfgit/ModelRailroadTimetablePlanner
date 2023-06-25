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

#ifndef RSERRORTREEMODEL_H
#define RSERRORTREEMODEL_H

#ifdef ENABLE_BACKGROUND_MANAGER

#    include "rs_error_data.h"
#    include "utils/singledepthtreemodelhelper.h"

class RsErrorTreeModel;
typedef SingleDepthTreeModelHelper<RsErrorTreeModel, RsErrors::RSErrorMap, RsErrors::RSErrorData>
  RsErrorTreeModelBase;

// TODO: make on-demand
class RsErrorTreeModel : public RsErrorTreeModelBase
{
    Q_OBJECT
public:
    enum Columns
    {
        JobName = 0,
        StationName,
        Arrival,
        Description,
        NCols
    };

    RsErrorTreeModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    void setErrors(const QMap<db_id, RsErrors::RSErrorList> &data);

    void mergeErrors(const QMap<db_id, RsErrors::RSErrorList> &data);

    void clear();

private slots:
    void onRSInfoChanged(db_id rsId);
};

#endif // ENABLE_BACKGROUND_MANAGER

#endif // RSERRORTREEMODEL_H
