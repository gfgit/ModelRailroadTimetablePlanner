#ifndef DIRECTIONTYPE_H
#define DIRECTIONTYPE_H

#include <QCoreApplication>

enum class Direction: bool
{
    Left = 0,
    Right = 1
};

static const char* DirectionNamesTable[] = {
    QT_TRANSLATE_NOOP("DirectionNames", "Left"),
    QT_TRANSLATE_NOOP("DirectionNames", "Right")
};

class DirectionNames
{
    Q_DECLARE_TR_FUNCTIONS(DirectionNames)

public:
    static inline QString name(Direction d)
    {
        if(size_t(d) >= sizeof (DirectionNamesTable)/sizeof (DirectionNamesTable[0]))
            return QString();
        return tr(DirectionNamesTable[int(d)]);
    }
};

#endif // DIRECTIONTYPE_H
