#ifndef TYPES_H
#define TYPES_H

#include <QtGlobal>

//64 bit signed integer used for SQL Primary Key ID
typedef qint64 db_id;

enum class RsType : qint8
{
    Engine = 0,
    FreightWagon,
    Coach ,
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

enum class StopType {
    ToggleType = -1, //Used as flag in StopModel::setStopTypeRange()
    Normal = 0,
    Transit,
    First,
    Last
};

typedef enum {
    Uncoupled = 0,
    Coupled   = 1
} RsOp;

//FIXME: remove, use utils::RailwaySegmentType
enum class LineType
{
    NonElectric = 0,
    Electric = 1
};

enum class JobCategory : qint8
{
    FREIGHT = 0,
    LIS, //Locomotiva In Spostamento (Rimando)
    POSTAL,

    REGIONAL, //Passenger
    FAST_REGIONAL, //RV - Regionale veloce
    LOCAL,
    INTERCITY,
    EXPRESS,
    DIRECT,
    HIGH_SPEED,

    NCategories
};

constexpr JobCategory LastFreightCategory = JobCategory::POSTAL;
constexpr JobCategory FirstPassengerCategory = JobCategory::REGIONAL;

struct JobEntry
{
    db_id jobId;
    JobCategory category;
};

struct JobStopEntry
{
    db_id stopId = 0;
    db_id jobId = 0;
    JobCategory category = JobCategory::NCategories;
};

#endif // TYPES_H
