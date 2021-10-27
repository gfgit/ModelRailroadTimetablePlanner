#ifndef DEFS_H
#define DEFS_H

#include <QLatin1String>
#include "graph/linegraphtypes.h"

namespace Print {

enum OutputType {
    Native = 0,
    Pdf,
    Svg,
    NTypes
};

//Implemented in printwizard.cpp
QString getOutputTypeName(OutputType type);

//Place holders for file names
const QLatin1String phNameUnderscore = QLatin1String("%n");
const QLatin1String phNameKeepSpaces = QLatin1String("%N");
const QLatin1String phType = QLatin1String("%t");
const QLatin1String phProgressive = QLatin1String("%i");

const QString phDefaultPattern = phType + QLatin1Char('_') + phNameUnderscore;

//Implemented in printwizard.cpp
QString getFileName(const QString& baseDir, const QString& pattern, const QString& extension,
                    const QString& name, LineGraphType type, int i);

}

#endif // DEFS_H
