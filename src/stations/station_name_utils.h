#ifndef STATION_NAME_UTILS_H
#define STATION_NAME_UTILS_H

#include <QCoreApplication>

#include "station_utils.h"

namespace utils {

static const char* StationTypeNamesTable[] = {
    QT_TRANSLATE_NOOP("StationUtils", "Normal"),
    QT_TRANSLATE_NOOP("StationUtils", "Simple Stop"),
    QT_TRANSLATE_NOOP("StationUtils", "Junction")
};

class StationUtils
{
    Q_DECLARE_TR_FUNCTIONS(StationUtils)

public:
    static inline QString name(StationType t)
    {
        if(t >= StationType::NTypes)
            return QString();
        return tr(StationTypeNamesTable[int(t)]);
    }

    static inline QString name(Side s)
    {
        return s == Side::East ? tr("East") : tr("West");
    }
};

} // namespace utils

#endif // STATION_NAME_UTILS_H
