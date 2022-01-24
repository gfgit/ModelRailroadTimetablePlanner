#include "printwizard.h"

#include "selectionpage.h"
#include "printoptionspage.h"
#include "progresspage.h"

#include "sceneselectionmodel.h"
#include "graph/model/linegraphscene.h"

#include "printing/printworker.h"
#include <QThreadPool>

#include <QMessageBox>
#include <QPushButton>
#include "utils/owningqpointer.h"

#include <QPrinter>

QString Print::getOutputTypeName(Print::OutputType type)
{
    switch (type)
    {
    case Print::OutputType::Native:
        return PrintWizard::tr("Native Printer");
    case Print::OutputType::Pdf:
        return PrintWizard::tr("PDF");
    case Print::OutputType::Svg:
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
    printTask(nullptr),
    isStoppingTask(false)
{
    //Initialize to a default pattern
    printOpt.fileNamePattern = Print::phType + QLatin1Char('_') + Print::phNameUnderscore;

    printer = new QPrinter;
    printer->setOutputFormat(QPrinter::PdfFormat);

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

bool PrintWizard::validateCurrentPage()
{
    QWizardPage *curPage = currentPage();
    if(!curPage)
        return true;

    if(!curPage->validatePage())
        return false;

    //NOTE: needed because when going back and then again forward
    //initializePage() is not called again
    //So we need to manually initialize again next page
    int id = currentId();
    if(id == 0)
    {
        //Setup PrintOptionsPage
        PrintOptionsPage *optionsPage = static_cast<PrintOptionsPage *>(page(1));
        optionsPage->setupPage();
    }
    else if(id == 1)
    {
        //After PrintOptionsPage start task
        startPrintTask();
    }

    return true;
}

QPrinter *PrintWizard::getPrinter() const
{
    return printer;
}

void PrintWizard::setOutputType(Print::OutputType type)
{
    printOpt.outputType = type;
    validatePrintOptions();
}

PrintHelper::PageLayoutOpt PrintWizard::getScenePageLay() const
{
    return scenePageLay;
}

void PrintWizard::setScenePageLay(const PrintHelper::PageLayoutOpt &newScenePageLay)
{
    scenePageLay = newScenePageLay;
}

Print::PrintBasicOptions PrintWizard::getPrintOpt() const
{
    return printOpt;
}

void PrintWizard::setPrintOpt(const Print::PrintBasicOptions &newPrintOpt)
{
    printOpt = newPrintOpt;
    validatePrintOptions();
}

IGraphScene *PrintWizard::getFirstScene()
{
    if(!selectionModel->startIteration())
        return nullptr;

    SceneSelectionModel::Entry entry = selectionModel->getNextEntry();
    if(!entry.objectId)
        return nullptr;

    LineGraphScene *scene = new LineGraphScene(mDb);
    scene->loadGraph(entry.objectId, entry.type);
    return scene;
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

    validatePrintOptions();

    printTask = new PrintWorker(mDb, this);
    printTask->setPrintOpt(printOpt);
    printTask->setScenePageLay(scenePageLay);
    printTask->setSelection(selectionModel);
    printTask->setPrinter(printer);

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

void PrintWizard::validatePrintOptions()
{
    if(printOpt.outputType == Print::OutputType::Svg)
    {
        //Svg can only be printed in multiple files
        printOpt.useOneFileForEachScene = true;

        //Svg will always be on single page
        printOpt.printSceneInOnePage = true;
    }
    else if(printOpt.outputType == Print::OutputType::Native)
    {
        //Printers always need to split in multiple pages
        printOpt.printSceneInOnePage = false;
    }

    //Update printer output format
    printer->setOutputFormat(printOpt.outputType == Print::OutputType::Native ? QPrinter::NativeFormat : QPrinter::PdfFormat);
}
