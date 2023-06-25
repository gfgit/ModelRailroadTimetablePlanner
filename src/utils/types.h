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

#ifndef TYPES_H
#define TYPES_H

#include <QtGlobal>

// 64 bit signed integer used for SQL Primary Key ID
typedef qint64 db_id;

enum class RsType : qint8
{
    Engine = 0,
    FreightWagon,
    Coach,
    NTypes
};

enum class RsEngineSubType : qint8
{
    Invalid = 0,
    Electric,
    Diesel,
    Steam,
    NTypes
};

enum class StopType
{
    ToggleType = -1, // Used as flag in StopModel::setStopTypeRange()
    Normal     = 0,
    Transit,
    First,
    Last
};

enum class RsOp
{
    Uncoupled = 0,
    Coupled   = 1
};

enum class JobCategory : qint8
{
    FREIGHT = 0,
    LIS, // Locomotiva In Spostamento (Rimando)
    POSTAL,

    REGIONAL,      // Passenger
    FAST_REGIONAL, // RV - Regionale veloce
    LOCAL,
    INTERCITY,
    EXPRESS,
    DIRECT,
    HIGH_SPEED,

    NCategories
};

constexpr JobCategory LastFreightCategory    = JobCategory::POSTAL;
constexpr JobCategory FirstPassengerCategory = JobCategory::REGIONAL;

struct JobEntry
{
    db_id jobId          = 0;
    JobCategory category = JobCategory::NCategories;
};

struct JobStopEntry
{
    db_id stopId         = 0;
    db_id jobId          = 0;
    JobCategory category = JobCategory::NCategories;
};

#endif // TYPES_H
