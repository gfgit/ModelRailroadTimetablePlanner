#include "printwizard.h"

#include "selectionpage.h"
#include "printoptionspage.h"
#include "progresspage.h"

#include "sceneselectionmodel.h"

#include "printworker.h"
#include <QThreadPool>

#include <QMessageBox>
#include <QPushButton>
#include "utils/owningqpointer.h"

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
    type(Print::Native),
    printTask(nullptr),
    isStoppingTask(false)
{
    printer = new QPrinter;
    selectionModel = new SceneSelectionModel(mDb, this);

    setPage(0, new PrintSelectionPage(this));
    setPage(1, new PrintOptionsPage(this));
    progressPage = new PrintProgressPage(this);
    setPage(2, progressPage);

    setWindowTitle(tr("Print Wizard"));
}

PrintWizard::~PrintWizard()
{
    abortPrintTask();
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

bool PrintWizard::event(QEvent *e)
{
    if(e->type() == PrintProgressEvent::_Type)
    {
        PrintProgressEvent *ev = static_cast<PrintProgressEvent *>(e);
        ev->setAccepted(true);

        if(ev->task == printTask)
        {
            if(ev->progress < 0)
            {
                //Task finished, delete it
                delete printTask;
                printTask = nullptr;
            }

            QString description;
            if(ev->progress == PrintProgressEvent::ProgressError)
                description = tr("Error");
            else if(ev->progress == PrintProgressEvent::ProgressAbortedByUser)
                description = tr("Canceled");
            else
                description = tr("Printing %1...").arg(ev->descriptionOrError);

            progressPage->handleProgress(ev->progress, description);

            if(ev->progress == PrintProgressEvent::ProgressError)
            {
                handleProgressError(ev->descriptionOrError);
            }
        }

        return true;
    }

    return QWizard::event(e);
}

void PrintWizard::done(int result)
{
    if(result == QDialog::Rejected)
    {
        if(!isStoppingTask)
        {
            OwningQPointer<QMessageBox> msgBox = new QMessageBox(this);
            msgBox->setIcon(QMessageBox::Question);
            msgBox->setWindowTitle(tr("Abort Printing?"));
            msgBox->setText(tr("Do you want to cancel printing?\n"
                               "Some items may have already be printed."));
            QPushButton *abortBut = msgBox->addButton(QMessageBox::Abort);
            QPushButton *noBut = msgBox->addButton(QMessageBox::No);
            msgBox->setDefaultButton(noBut);
            msgBox->setEscapeButton(noBut); //Do not Abort if dialog is closed by Esc or X window button
            msgBox->exec();
            bool abortClicked = msgBox && msgBox->clickedButton() == abortBut;
            if(!abortClicked)
                return;

            if(printTask)
            {
                printTask->stop();
                isStoppingTask = true;
                progressPage->handleProgress(0, tr("Aborting..."));
                return;
            }
        }
        else
        {
            if(printTask)
                return; //Already sent 'stop', just wait
        }
    }

    QWizard::done(result);
}

void PrintWizard::startPrintTask()
{
    abortPrintTask();

    printTask = new PrintWorker(mDb, this);
    printTask->setSelection(selectionModel);
    printTask->setOutputType(type);
    printTask->setPrinter(printer);
    printTask->setFileOutput(fileOutput, differentFiles);
    printTask->setFilePattern(filePattern);

    QThreadPool::globalInstance()->start(printTask);

    progressPage->handleProgressStart(printTask->getMaxProgress());
    progressPage->handleProgress(0, tr("Starting..."));
}

void PrintWizard::abortPrintTask()
{
    if(printTask)
    {
        printTask->stop();
        printTask->cleanup();
        printTask = nullptr;
    }
}

void PrintWizard::handleProgressError(const QString &errMsg)
{
    OwningQPointer<QMessageBox> msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Warning);
    auto tryAgainBut = msgBox->addButton(tr("Try again"), QMessageBox::YesRole);
    msgBox->addButton(QMessageBox::Abort);
    msgBox->setDefaultButton(tryAgainBut);
    msgBox->setText(errMsg);
    msgBox->setWindowTitle(tr("Print Error"));

    msgBox->exec();
    if(!msgBox)
        return;

    if(msgBox->clickedButton() == tryAgainBut)
    {
        //Go back to options page
        back();
    }
}
