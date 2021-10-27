#include "printwizard.h"

#include "selectionpage.h"
#include "printoptionspage.h"
#include "progresspage.h"

#include "sceneselectionmodel.h"

#include <QPrinter>

QString Print::getOutputTypeName(Print::OutputType type)
{
    switch (type)
    {
    case Print::Native:
        return PrintWizard::tr("Native Printer");
    case Print::Pdf:
        return PrintWizard::tr("PDF");
    case Print::Svg:
        return PrintWizard::tr("SVG");
    default:
        break;
    }
    return QString();
}

QString Print::getFileName(const QString& baseDir, const QString& pattern, const QString& extension,
                           const QString& name, LineGraphType type, int i)
{
    QString result = pattern;
    if(result.contains(phNameUnderscore))
    {
        //Replace spaces with underscores
        QString nameUnderscores = name;
        nameUnderscores.replace(' ', '_');
        result.replace(phNameUnderscore, nameUnderscores);
    }

    result.replace(phNameKeepSpaces, name);
    result.replace(phType, utils::getLineGraphTypeName(type));
    result.replace(phProgressive, QString::number(i).rightJustified(2, '0'));

    if(!baseDir.endsWith('/'))
        result.prepend('/');
    result.prepend(baseDir);

    result.append(extension);

    return result;
}

PrintWizard::PrintWizard(sqlite3pp::database &db, QWidget *parent) :
    QWizard (parent),
    mDb(db),
    printer(nullptr),
    filePattern(Print::phDefaultPattern),
    differentFiles(false),
    type(Print::Native)
{
    printer = new QPrinter;
    selectionModel = new SceneSelectionModel(mDb, this);

    setPage(0, new PrintSelectionPage(this));
    setPage(1, new PrintOptionsPage(this));
    setPage(2, new PrintProgressPage(this));

    setWindowTitle(tr("Print Wizard"));
}

PrintWizard::~PrintWizard()
{
    delete printer;
}

Print::OutputType PrintWizard::getOutputType() const
{
    return type;
}

void PrintWizard::setOutputType(Print::OutputType out)
{
    type = out;
}

QString PrintWizard::getOutputFile() const
{
    return fileOutput;
}

void PrintWizard::setOutputFile(const QString &fileName)
{
    fileOutput = fileName;
}

bool PrintWizard::getDifferentFiles() const
{
    return differentFiles;
}

void PrintWizard::setDifferentFiles(bool newDifferentFiles)
{
    differentFiles = newDifferentFiles;
}

QPrinter *PrintWizard::getPrinter() const
{
    return printer;
}

const QString &PrintWizard::getFilePattern() const
{
    return filePattern;
}

void PrintWizard::setFilePattern(const QString &newFilePattern)
{
    filePattern = newFilePattern;
}
