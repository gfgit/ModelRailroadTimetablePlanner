#ifndef STATION_UITLS_H
#define STATION_UITLS_H

#include "utils/types.h"

namespace utils
{

enum class StationType : qint8
{
    Normal = 0, //Normal station
    SimpleStop = 1, //Trains can stop but cannot be origin or destination
    Junction = 2, //This is not a real station but instead a junction between 2 lines
    NTypes
};

//TODO: is this useful???
enum class GateType : qint8
{
    Unknown = 0,
    Entrance = 1 << 0,
    Exit     = 1 << 1,
    Bidirectional = (Entrance | Exit),

    LeftHandTraffic = 1 << 2,
    RightHandTraffic = 1 << 3,
    MultipleTraffic = (LeftHandTraffic | RightHandTraffic)
};

enum class GateSide : qint8
{
    East = 0,
    West = 1
};

//NOTE: a track can be for passenger and freight traffic at the same time or none of them
enum class StationTrackType : qint8
{
    Electrified = 1 << 0, //Electric engines are allowed
    Passenger   = 1 << 1, //For passenger traffic
    Freight     = 1 << 2, //For freight traffic
    Through     = 1 << 3 //For non-stopping trains
};

enum class RailwaySegmentType : qint8
{
    Electrified = 1 << 0, //Electric engines are allowed

    LeftHandTraffic = 1 << 2,
    RightHandTraffic = 1 << 3,
    MultipleTraffic = (LeftHandTraffic | RightHandTraffic)
};

} // namespace utils

#endif // STATION_UITLS_H
