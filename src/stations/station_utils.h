#ifndef STATION_UITLS_H
#define STATION_UITLS_H

#include "utils/types.h"
#include <QString>

#include <QString>

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
    Entrance = 1 << 0, //NOTE: at least Entrance or Exit
    Exit     = 1 << 1,
    Bidirectional = (Entrance | Exit),

    LeftHandTraffic = 1 << 2,
    RightHandTraffic = 1 << 3,
    MultipleTraffic = (LeftHandTraffic | RightHandTraffic)
};

enum class Side : qint8
{
    West = 0,
    East = 1,
    NSides
};

//NOTE: a track can be for passenger and freight traffic at the same time or none of them
//      just set platf_length_cm TO non-zero, same for freight_length_cm
enum class StationTrackType : qint8
{
    Electrified = 1 << 0, //Electric engines are allowed
    Through     = 1 << 1 //For non-stopping trains
};

enum class RailwaySegmentType : qint8
{
    Electrified = 1 << 0, //Electric engines are allowed

    LeftHandTraffic = 1 << 2,
    RightHandTraffic = 1 << 3,
    MultipleTraffic = (LeftHandTraffic | RightHandTraffic)
};

struct RailwaySegmentGateInfo
{
    db_id gateId = 0;
    db_id stationId = 0;
    QString stationName;
    QChar gateLetter;
};

struct RailwaySegmentInfo
{
    db_id segmentId = 0;
    QString segmentName;
    int distanceMeters = 10000; //10 Km
    int maxSpeedKmH = 120;
    RailwaySegmentType type;

    RailwaySegmentGateInfo from;
    RailwaySegmentGateInfo to;
};

} // namespace utils

#endif // STATION_UITLS_H
