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

#include "railwaysegmentsmodel.h"

#include "utils/delegates/sql/pageditemmodelhelper_impl.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "utils/delegates/kmspinbox/kmutils.h"

#include "stations/station_name_utils.h"

#include <QBrush>

#include <QDebug>

RailwaySegmentsModel::RailwaySegmentsModel(sqlite3pp::database &db, QObject *parent) :
    BaseClass(500, db, parent),
    filterFromStationId(0)
{
    connect(Session, &MeetingSession::stationNameChanged, this, &IPagedItemModel::clearCache_slot);
    sortColumn = NameCol;
}

QVariant RailwaySegmentsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        switch (role)
        {
        case Qt::DisplayRole:
        {
            switch (section)
            {
            case NameCol:
                return tr("Name");
            case FromStationCol:
                return tr("From");
            case FromGateCol:
                return tr("Gate");
            case ToStationCol:
                return tr("To");
            case ToGateCol:
                return tr("To Gate");
            case MaxSpeedCol:
                return tr("Max. Speed");
            case DistanceCol:
                return tr("Distance Km");
            case IsElectrifiedCol:
                return tr("Electrified");
            }
            break;
        }
        }
    }
    else if(role == Qt::DisplayRole)
    {
        return section + curPage * ItemsPerPage + 1;
    }

    return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant RailwaySegmentsModel::data(const QModelIndex &idx, int role) const
{
    const int row = idx.row();
    if (!idx.isValid() || row >= curItemCount || idx.column() >= NCols)
        return QVariant();

    if(row < cacheFirstRow || row >= cacheFirstRow + cache.size())
    {
        //Fetch above or below current cache
        const_cast<RailwaySegmentsModel *>(this)->fetchRow(row);

        //Temporarily return null
        return role == Qt::DisplayRole ? QVariant("...") : QVariant();
    }

    const RailwaySegmentItem& item = cache.at(row - cacheFirstRow);

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case NameCol:
            return item.segmentName;
        case FromStationCol:
            return item.fromStationName;
        case FromGateCol:
            return item.fromGateLetter;
        case ToStationCol:
            return item.toStationName;
        case ToGateCol:
            return item.toGateLetter;
        case MaxSpeedCol:
            return QStringLiteral("%1 km/h").arg(item.maxSpeedKmH);
        case DistanceCol:
            return utils::kmNumToText(item.distanceMeters);
        }
        break;
    }
    case Qt::ToolTipRole:
    {
        switch (idx.column())
        {
        case FromStationCol:
        case ToStationCol:
        {
            if(item.reversed)
                return tr("Segment <b>%1</b> is shown reversed.").arg(item.segmentName);
            break;
        }
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        switch (idx.column())
        {
        case FromGateCol:
        case ToGateCol:
            return Qt::AlignCenter;
        case MaxSpeedCol:
        case DistanceCol:
            return Qt::AlignRight + Qt::AlignVCenter;
        }
        break;
    }
    case Qt::BackgroundRole:
    {
        switch (idx.column())
        {
        case FromStationCol:
        case ToStationCol:
        {
            if(item.reversed)
            {
                //Light red
                QBrush b(qRgb(255, 110, 110));
                return b;
            }
            break;
        }
        }
        break;
    }
    case Qt::CheckStateRole:
    {
        switch (idx.column())
        {
        case IsElectrifiedCol:
            return item.type.testFlag(utils::RailwaySegmentType::Electrified) ? Qt::Checked : Qt::Unchecked;
        }
        break;
    }
    }

    return QVariant();
}

Qt::ItemFlags RailwaySegmentsModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    if(idx.row() < cacheFirstRow || idx.row() >= cacheFirstRow + cache.size())
        return f; //Not fetched yet

    if(idx.column() == IsElectrifiedCol)
        f.setFlag(Qt::ItemIsUserCheckable);

    return f;
}

void RailwaySegmentsModel::setSortingColumn(int col)
{
    if(sortColumn == col || col == FromGateCol || col == ToGateCol || col == IsElectrifiedCol)
        return;

    clearCache();
    sortColumn = col;

    QModelIndex first = index(0, 0);
    QModelIndex last = index(curItemCount - 1, NCols - 1);
    emit dataChanged(first, last);
}

db_id RailwaySegmentsModel::getFilterFromStationId() const
{
    return filterFromStationId;
}

void RailwaySegmentsModel::setFilterFromStationId(const db_id &value)
{
    filterFromStationId = value;
    refreshData(true);
}

qint64 RailwaySegmentsModel::recalcTotalItemCount()
{
    query q(mDb);
    if(filterFromStationId)
    {
        q.prepare("SELECT COUNT(1) FROM railway_segments s"
                  " JOIN station_gates g1 ON g1.id=s.in_gate_id"
                  " JOIN station_gates g2 ON g2.id=s.out_gate_id"
                  " WHERE g1.station_id=?1 OR g2.station_id=?1");
        q.bind(1, filterFromStationId);
    }else{
        q.prepare("SELECT COUNT(1) FROM railway_segments");
    }

    q.step();
    const qint64 count = q.getRows().get<qint64>(0);
    return count;
}

void RailwaySegmentsModel::internalFetch(int first, int sortCol, int valRow, const QVariant &val)
{
    query q(mDb);

    int offset = first - valRow + curPage * ItemsPerPage;
    bool reverse = false;

    if(valRow > first)
    {
        offset = 0;
        reverse = true;
    }

    qDebug() << "Fetching:" << first << "ValRow:" << valRow << val << "Offset:" << offset << "Reverse:" << reverse;

    const char *whereCol = nullptr;

    QByteArray sql = "SELECT s.id, s.name, s.max_speed_kmh, s.type, s.distance_meters,"
                     "g1.station_id, g2.station_id,"
                     "s.in_gate_id, g1.name, st1.name,"
                     "s.out_gate_id,g2.name, st2.name"
                     " FROM railway_segments s"
                     " JOIN station_gates g1 ON g1.id=s.in_gate_id"
                     " JOIN station_gates g2 ON g2.id=s.out_gate_id"
                     " JOIN stations st1 ON st1.id=g1.station_id"
                     " JOIN stations st2 ON st2.id=g2.station_id";
    switch (sortCol)
    {
    case NameCol:
    {
        whereCol = "s.name"; //Order by 1 column, no where clause
        break;
    }
    case FromStationCol:
    {
        whereCol = "st1.name";
        break;
    }
    case ToStationCol:
    {
        whereCol = "st2.name";
        break;
    }
    case MaxSpeedCol:
    {
        whereCol = "s.max_speed_kmh";
        break;
    }
    case DistanceCol:
    {
        whereCol = "s.distance_meters";
        break;
    }
    }

    if(val.isValid())
    {
        sql += " WHERE ";
        sql += whereCol;
        if(reverse)
            sql += "<?3";
        else
            sql += ">?3";
    }

    if(filterFromStationId)
    {
        sql += " WHERE g1.station_id=?4 OR g2.station_id=?4";
    }

    sql += " ORDER BY ";
    sql += whereCol;

    if(reverse)
        sql += " DESC";

    sql += " LIMIT ?1";
    if(offset)
        sql += " OFFSET ?2";

    q.prepare(sql);
    q.bind(1, BatchSize);
    if(offset)
        q.bind(2, offset);

    if(filterFromStationId)
        q.bind(4, filterFromStationId);

    //    if(val.isValid())
    //    {
    //        switch (sortCol)
    //        {
    //        case LineNameCol:
    //        {
    //            q.bind(3, val.toString());
    //            break;
    //        }
    //        }
    //    }

    QVector<RailwaySegmentItem> vec(BatchSize);

    auto it = q.begin();
    const auto end = q.end();

    int i = reverse ? BatchSize - 1 : 0;
    const int increment = reverse ? -1 : 1;

    for(; it != end; ++it)
    {
        auto r = *it;
        RailwaySegmentItem &item = vec[i];
        item.segmentId = r.get<db_id>(0);
        item.segmentName = r.get<QString>(1);
        item.maxSpeedKmH = r.get<int>(2);
        item.type = utils::RailwaySegmentType(r.get<int>(3));
        item.distanceMeters = r.get<int>(4);

        item.fromStationId = r.get<db_id>(5);
        item.toStationId = r.get<db_id>(6);

        item.fromGateId = r.get<db_id>(7);
        item.fromGateLetter = sqlite3_column_text(q.stmt(), 8)[0];
        item.fromStationName = r.get<QString>(9);

        item.toGateId = r.get<db_id>(10);
        item.toGateLetter = sqlite3_column_text(q.stmt(), 11)[0];
        item.toStationName = r.get<QString>(12);
        item.reversed = false;

        if(filterFromStationId)
        {
            if(filterFromStationId == item.toStationId)
            {
                //Always show filter station as 'From'
                qSwap(item.fromStationId, item.toStationId);
                qSwap(item.fromGateId, item.toGateId);
                qSwap(item.fromStationName, item.toStationName);
                qSwap(item.fromGateLetter, item.toGateLetter);
                item.reversed = true;
            }
            //item.fromStationName.clear(); //Save some memory???
        }

        i += increment;
    }

    if(reverse && i > -1)
        vec.remove(0, i + 1);
    else if(i < BatchSize)
        vec.remove(i, BatchSize - i);

    postResult(vec, first);
}
