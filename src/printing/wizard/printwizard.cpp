#include "printwizard.h"

#include "selectionpage.h"
#include "printoptionspage.h"
#include "progresspage.h"

#include "sceneselectionmodel.h"

#include "printing/printworkerhandler.h"

#include "utils/owningqpointer.h"
#include <QMessageBox>
#include <QPushButton>

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
                           const QString& name, const QString &type, int i)
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
    result.replace(phType, type);
    result.replace(phProgressive, QString::number(i).rightJustified(2, '0'));

    if(!baseDir.endsWith('/'))
        result.prepend('/');
    result.prepend(baseDir);

    result.append(extension);

    return result;
}

void Print::validatePrintOptions(Print::PrintBasicOptions& printOpt, QPrinter *printer)
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

    if(printer)
    {
        //Update printer output format
        printer->setOutputFormat(printOpt.outputType == Print::OutputType::Native ? QPrinter::NativeFormat : QPrinter::PdfFormat);
    }
}

bool Print::askUserToAbortPrinting(bool wasAlreadyStarted, QWidget *parent)
{
    OwningQPointer<QMessageBox> msgBox = new QMessageBox(parent);
    msgBox->setIcon(QMessageBox::Question);
    msgBox->setWindowTitle(PrintWizard::tr("Abort Printing?"));

    QString msg;
    if(wasAlreadyStarted)
        msg = PrintWizard::tr("Do you want to abort printing?\n"
                              "Some items may have already be printed.");
    else
        msg = PrintWizard::tr("Do you want to cancel printing?");

    msgBox->setText(msg);
    QPushButton *abortBut = msgBox->addButton(QMessageBox::Abort);
    QPushButton *noBut = msgBox->addButton(QMessageBox::No);
    msgBox->setDefaultButton(noBut);
    msgBox->setEscapeButton(noBut); //Do not Abort if dialog is closed by Esc or X window button
    msgBox->exec();
    bool abortClicked = msgBox && msgBox->clickedButton() == abortBut;
    return abortClicked;
}

bool Print::askUserToTryAgain(const QString& errMsg, QWidget *parent)
{
    OwningQPointer<QMessageBox> msgBox = new QMessageBox(parent);
    msgBox->setIcon(QMessageBox::Warning);
    auto tryAgainBut = msgBox->addButton(PrintWizard::tr("Try again"), QMessageBox::YesRole);
    msgBox->addButton(QMessageBox::Abort);
    msgBox->setDefaultButton(tryAgainBut);
    msgBox->setText(errMsg);
    msgBox->setWindowTitle(PrintWizard::tr("Print Error"));

    msgBox->exec();
    if(!msgBox)
        return false;

    bool shouldTryAgain = msgBox->clickedButton() == tryAgainBut;
    return shouldTryAgain;
}

PrintWizard::PrintWizard(sqlite3pp::database &db, QWidget *parent) :
    QWizard (parent),
    mDb(db),
    printer(nullptr)
{
    printTaskHandler = new PrintWorkerHandler(mDb, this);
    connect(printTaskHandler, &PrintWorkerHandler::progressMaxChanged,
            this, &PrintWizard::progressMaxChanged);
    connect(printTaskHandler, &PrintWorkerHandler::progressChanged,
            this, &PrintWizard::progressChanged);
    connect(printTaskHandler, &PrintWorkerHandler::progressFinished,
            this, &PrintWizard::handleProgressFinished);

    printer = new QPrinter;
    printer->setOutputFormat(QPrinter::PdfFormat);

    //Initialize to a default pattern
    Print::PrintBasicOptions printOpt;
    printOpt.fileNamePattern = Print::phType + QLatin1Char('_') + Print::phNameUnderscore;
    printTaskHandler->setOptions(printOpt, printer);

    //Remove page margins
    QPageLayout printerPageLay = printer->pageLayout();
    printerPageLay.setMode(QPageLayout::FullPageMode);
    printerPageLay.setMargins(QMarginsF());
    printer->setPageLayout(printerPageLay);

    //Apply page size to scene layout
    Print::PageLayoutOpt scenePageLay;
    PrintHelper::applyPageSize(printerPageLay.pageSize(), printerPageLay.orientation(), scenePageLay);
    printTaskHandler->setScenePageLay(scenePageLay);

    selectionModel = new SceneSelectionModel(mDb, this);

    setPage(0, new PrintSelectionPage(this));
    setPage(1, new PrintOptionsPage(this));
    progressPage = new PrintProgressPage(this);
    setPage(2, progressPage);

    setWindowTitle(tr("Print Wizard"));
}

PrintWizard::~PrintWizard()
{
    printTaskHandler->abortPrintTask();
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
        printTaskHandler->startPrintTask(printer, selectionModel);
    }

    return true;
}

QPrinter *PrintWizard::getPrinter() const
{
    return printer;
}

void PrintWizard::setOutputType(Print::OutputType type)
{
    Print::PrintBasicOptions printOpt = printTaskHandler->getOptions();
    printOpt.outputType = type;
    printTaskHandler->setOptions(printOpt, printer);
}

Print::PageLayoutOpt PrintWizard::getScenePageLay() const
{
    return printTaskHandler->getScenePageLay();
}

void PrintWizard::setScenePageLay(const Print::PageLayoutOpt &newScenePageLay)
{
    printTaskHandler->setScenePageLay(newScenePageLay);
}

Print::PrintBasicOptions PrintWizard::getPrintOpt() const
{
    return printTaskHandler->getOptions();
}

void PrintWizard::setPrintOpt(const Print::PrintBasicOptions &newPrintOpt)
{
    printTaskHandler->setOptions(newPrintOpt, printer);
}

IGraphScene *PrintWizard::getFirstScene()
{
    if(!selectionModel->startIteration())
        return nullptr;

    IGraphSceneCollection::SceneItem item = selectionModel->getNextItem();
    if(!item.scene)
        return nullptr;

    //Caller is responsible for deleting
    return item.scene;
}

bool PrintWizard::taskRunning() const
{
    return printTaskHandler->taskIsRunning();
}

void PrintWizard::done(int result)
{
    //When we finish, we disable Cancel button
    //If finished do not prompt about aborting printing
    //Tasks is already cleaned up
    const bool printingFinished = !button(CancelButton)->isEnabled();

    if(result == QDialog::Rejected && !printingFinished)
    {
        if(!printTaskHandler->waitingForTaskToStop())
        {
            //Ask user if he wants to abort printing
            if(!Print::askUserToAbortPrinting(printTaskHandler->taskIsRunning(), this))
                return;

            if(printTaskHandler->taskIsRunning())
            {
                printTaskHandler->stopTaskGracefully();
                return;
            }
        }
        else
        {
            if(printTaskHandler->taskIsRunning())
                return; //Already sent 'stop', just wait
        }
    }

    QWizard::done(result);
}

void PrintWizard::progressMaxChanged(int max)
{
    progressPage->handleProgressStart(max);
}

void PrintWizard::progressChanged(int val, const QString &msg)
{
    progressPage->handleProgress(val, msg);
}

void PrintWizard::handleProgressFinished(bool success, const QString &errMsg)
{
    if(!success)
    {
        if(Print::askUserToTryAgain(errMsg, this))
        {
            //Go back to options page
            back();
            return;
        }
    }

    //When finished, disable Cancel button.
    button(CancelButton)->setEnabled(false);
}
