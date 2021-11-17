#include "linegraphtypes.h"

#include <QCoreApplication>

class LineGraphTypeNames
{
    Q_DECLARE_TR_FUNCTIONS(LineGraphTypeNames)
public:
    static const char *texts[];
};

const char *LineGraphTypeNames::texts[] = {
    QT_TRANSLATE_NOOP("LineGraphTypeNames", "No Graph"),
    QT_TRANSLATE_NOOP("LineGraphTypeNames", "Station"),
    QT_TRANSLATE_NOOP("LineGraphTypeNames", "Segment"),
    QT_TRANSLATE_NOOP("LineGraphTypeNames", "Line")
};

QString utils::getLineGraphTypeName(LineGraphType type)
{
    if(type >= LineGraphType::NTypes)
        return QString();
    return LineGraphTypeNames::tr(LineGraphTypeNames::texts[int(type)]);
}
