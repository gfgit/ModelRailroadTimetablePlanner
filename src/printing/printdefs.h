#ifndef DEFS_H
#define DEFS_H

#include <QString>
#include "graph/linegraphtypes.h"

namespace Print {

enum class OutputType {
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

//Implemented in printwizard.cpp
QString getFileName(const QString& baseDir, const QString& pattern, const QString& extension,
                    const QString& name, LineGraphType type, int i);

struct PrintBasicOptions
{
    OutputType outputType = Print::OutputType::Pdf;
    QString fileNamePattern;
    QString filePath;
    bool useOneFileForEachScene = true;
    bool printSceneInOnePage = false;
};

}

#endif // DEFS_H
