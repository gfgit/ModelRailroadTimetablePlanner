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

typedef enum {
    ToggleType = -1, //Used as flag in StopModel::setStopTypeRange()
    Normal = 0,
    Transit,
    TransitLineChange, //FIXME: remove
    First,
    Last
} StopType;

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

typedef struct JobEntry_
{
    db_id jobId;
    JobCategory category;
} JobEntry;

typedef struct
{
    db_id stopId = 0;
    db_id jobId = 0;
    JobCategory category = JobCategory::FREIGHT;
} JobStopEntry;

#endif // TYPES_H
