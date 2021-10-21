#ifndef LINEGRAPHTYPES_H
#define LINEGRAPHTYPES_H

#include <QString>

/*!
 * \brief Enum to describe view content type
 */
enum class LineGraphType
{
    NoGraph = 0, //!< No content displayed
    SingleStation, //!< Show a single station
    RailwaySegment, //!< Show two adjacent stations and the segment in between
    RailwayLine, //!< Show a complete railway line (multiple adjacent segments)
    NTypes
};

namespace utils {

QString getLineGraphTypeName(LineGraphType type);

} // namespace utils

#endif // LINEGRAPHTYPES_H
