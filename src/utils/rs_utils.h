#ifndef RS_UTILS_H
#define RS_UTILS_H

#include <QString>

#include "types.h"

namespace rs_utils
{
//Format RS number according to its type
QString formatNum(RsType type, int number);

QString formatName(const QString& model, int number, const QString& suffix, RsType type);
QString formatNameRef(const char *model, int modelSize, int number, const char *suffix, int suffixSize, RsType type);

}

#endif // RS_UTILS_H
