#include "shiftgraphprintdlg.h"

#include <QVBoxLayout>
#include "printing/helper/view/printeroptionswidget.h"

#include <QPushButton>
#include <QDialogButtonBox>

#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>

#include "shifts/shiftgraph/model/shiftgraphscenecollection.h"
#include "utils/scene/igraphscene.h"

#include <QPrinter>
#include "printing/helper/model/printworkerhandler.h"

#include "utils/files/openfileinfolder.h"

ShiftGraphPrintDlg::ShiftGraphPrintDlg(sqlite3pp::database &db, QWidget *parent) :
    QDialog(parent),
    mDb(db)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    //Setup Options Widget
    optionsWidget = new PrinterOptionsWidget;
    lay->addWidget(optionsWidget);
    connect(optionsWidget, &PrinterOptionsWidget::completeChanged,
            this, &ShiftGraphPrintDlg::updatePrintButton);

    //Setup Progress Group Box
    progressBox = new QGroupBox(tr("Progress:"));
    QVBoxLayout *progLay = new QVBoxLayout(progressBox);

    progressLabel = new QLabel;
    progLay->addWidget(progressLabel);
    progressBar = new QProgressBar;
    progLay->addWidget(progressBar);

    lay->addWidget(progressBox);

    //Dialog button box
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    lay->addWidget(buttonBox);

    //Change 'Ok' button to 'Print'
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Print"));

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setWindowTitle(tr("Print Shift Graph"));
    resize(500, 400);

    printTaskHandler = new PrintWorkerHandler(mDb, this);
    connect(printTaskHandler, &PrintWorkerHandler::progressMaxChanged,
            this, &ShiftGraphPrintDlg::progressMaxChanged);
    connect(printTaskHandler, &PrintWorkerHandler::progressChanged,
            this, &ShiftGraphPrintDlg::progressChanged);
    connect(printTaskHandler, &PrintWorkerHandler::progressFinished,
            this, &ShiftGraphPrintDlg::handleProgressFinished);

    //Initialize printer
    m_printer = new QPrinter;
    m_printer->setOutputFormat(QPrinter::PdfFormat);

    //Initialize to a default pattern
    Print::PrintBasicOptions printOpt;
    printOpt.fileNamePattern = Print::phNameUnderscore;
    printOpt.useOneFileForEachScene = false; //We have only 1 scene, so 1 file
    printTaskHandler->setOptions(printOpt, m_printer);

    //Remove page margins
    QPageLayout printerPageLay = m_printer->pageLayout();
    printerPageLay.setMode(QPageLayout::FullPageMode);
    printerPageLay.setMargins(QMarginsF());
    m_printer->setPageLayout(printerPageLay);

    //Apply page size to scene layout
    Print::PageLayoutOpt scenePageLay;
    PrintHelper::applyPageSize(printerPageLay.pageSize(), printerPageLay.orientation(), scenePageLay);
    printTaskHandler->setScenePageLay(scenePageLay);

    m_collection = new ShiftGraphSceneCollection(mDb);

    //Init options
    optionsWidget->setPrinter(m_printer);
    optionsWidget->setOptions(printOpt);
    optionsWidget->setScenePageLay(scenePageLay);

    //Create scene
    m_collection->startIteration();
    IGraphSceneCollection::SceneItem item = m_collection->getNextItem();

    item.scene->setParent(this); //Take ownership
    optionsWidget->setSourceScene(item.scene);

    updatePrintButton();
}

ShiftGraphPrintDlg::~ShiftGraphPrintDlg()
{
    printTaskHandler->abortPrintTask();

    //Delete collection only after aborting
    delete m_collection;
    m_collection = nullptr;

    //Reset scene
    optionsWidget->setSourceScene(nullptr);
}

void ShiftGraphPrintDlg::setOutputType(Print::OutputType type)
{
    Print::PrintBasicOptions printOpt = optionsWidget->getOptions();
    printOpt.outputType = type;
    optionsWidget->setOptions(printOpt);
}

void ShiftGraphPrintDlg::done(int res)
{
    //When we finish, we disable Cancel button
    //If finished do not prompt about aborting printing
    //Tasks is already cleaned up
    const bool printingFinished = !buttonBox->button(QDialogButtonBox::Cancel)->isEnabled();

    if(res == QDialog::Rejected && !printingFinished)
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
    else if(res == QDialog::Accepted && !printingFinished && !printTaskHandler->taskIsRunning())
    {
        //Start printing
        if(!optionsWidget->validateOptions())
            return; //User must set valid options

        printTaskHandler->setOptions(optionsWidget->getOptions(), m_printer);
        printTaskHandler->setScenePageLay(optionsWidget->getScenePageLay());

        printTaskHandler->startPrintTask(m_printer, m_collection);

        //Disable options and show progress
        optionsWidget->setEnabled(false);
        progressBox->setVisible(true);

        //Disable 'Print' while printing
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }

    if(printTaskHandler->taskIsRunning())
        return; //Task is running, cannot quit now

    QDialog::done(res);
}

void ShiftGraphPrintDlg::updatePrintButton()
{
    //Enable printing only if options is complete
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(optionsWidget->isComplete());
}

void ShiftGraphPrintDlg::progressMaxChanged(int max)
{
    progressBar->setMaximum(max);
}

void ShiftGraphPrintDlg::progressChanged(int val, const QString &msg)
{
    progressBar->setValue(val);
    progressLabel->setText(msg);
}

void ShiftGraphPrintDlg::handleProgressFinished(bool success, const QString &errMsg)
{
    if(!success)
    {
        if(Print::askUserToTryAgain(errMsg, this))
        {
            //Enable back options and hide progress
            optionsWidget->setEnabled(true);
            progressBox->setVisible(false);

            //Enable 'Print' button again
            buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            return;
        }
    }

    //When finished, disable Cancel button.
    buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);

    //Enable button 'Ok' whith default test
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    const Print::PrintBasicOptions printOpt = printTaskHandler->getOptions();
    if(printOpt.outputType != Print::OutputType::Native)
    {
        utils::OpenFileInFolderDlg::askUser(tr("Shift Graph Saved"), printOpt.filePath, this);
    }
}
