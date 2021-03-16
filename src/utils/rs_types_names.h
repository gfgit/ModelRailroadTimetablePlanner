#ifndef RS_TYPES_NAMES_H
#define RS_TYPES_NAMES_H

#include <QCoreApplication>
#include "types.h"

static const char* RsTypeNamesTable[] = {
    QT_TRANSLATE_NOOP("RsTypeNames", "Engine"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Freight Wagon"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Coach")
};

static const char* RsSubTypeNamesTable[] = {
    QT_TRANSLATE_NOOP("RsTypeNames", "Not set!"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Electric"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Diesel"),
    QT_TRANSLATE_NOOP("RsTypeNames", "Steam")
};


class RsTypeNames
{
    Q_DECLARE_TR_FUNCTIONS(RsTypeNames)

public:
    static inline QString name(RsType t)
    {
        if(t >= RsType::NTypes)
            return QString();
        return tr(RsTypeNamesTable[int(t)]);
    }

    static inline QString name(RsEngineSubType t)
    {
        if(t >= RsEngineSubType::NTypes)
            return QString();
        return tr(RsSubTypeNamesTable[int(t)]);
    }
};

#endif // RS_TYPES_NAMES_H
