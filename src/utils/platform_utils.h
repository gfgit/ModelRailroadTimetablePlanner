#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#include <QString>

namespace utils {

inline QString platformName(int platf)
{
    if(platf < 0) //Depot
        return QString::number(-platf) + QStringLiteral(" Depot");
    else
        return QString::number(platf + 1); //Main Platform
}

inline QString shortPlatformName(int platf)
{
    if(platf < 0) //Depot
        return QString::number(-platf) + QStringLiteral(" Dep");
    else
        return QString::number(platf + 1); //Main Platform
}

} //namespace utils

#endif // PLATFORM_UTILS_H
