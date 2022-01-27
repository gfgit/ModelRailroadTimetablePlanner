#include "printworkerhandler.h"

#include "printing/helper/model/printworker.h"
#include <QThreadPool>

PrintWorkerHandler::PrintWorkerHandler(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
    mDb(db),
    printTask(nullptr),
    isStoppingTask(false)
{

}

void PrintWorkerHandler::setOptions(const Print::PrintBasicOptions &opt, QPrinter *printer)
{
    printOpt = opt;
    Print::validatePrintOptions(printOpt, printer);
}

bool PrintWorkerHandler::event(QEvent *e)
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
            else if(ev->progress == PrintProgressEvent::ProgressMaxFinished)
                description = tr("Done!");
            else
                description = tr("Printing %1...").arg(ev->descriptionOrError);

            emit progressChanged(ev->progress, description);

            if(ev->progress < 0)
            {
                //Consider Abort succesful, on error send error message
                bool success = ev->progress != PrintProgressEvent::ProgressError;
                emit progressFinished(success, success ? description : ev->descriptionOrError);
            }
        }

        return true;
    }

    return QObject::event(e);
}

void PrintWorkerHandler::startPrintTask(QPrinter *printer, IGraphSceneCollection *collection)
{
    abortPrintTask();

    Print::validatePrintOptions(printOpt, printer);

    printTask = new PrintWorker(mDb, this);
    printTask->setPrintOpt(printOpt);
    printTask->setScenePageLay(scenePageLay);
    printTask->setCollection(collection);
    printTask->setPrinter(printer);

    QThreadPool::globalInstance()->start(printTask);

    //Start progress
    emit progressMaxChanged(printTask->getMaxProgress());
    emit progressChanged(0, tr("Starting..."));
}

void PrintWorkerHandler::abortPrintTask()
{
    if(printTask)
    {
        printTask->stop();
        printTask->cleanup();
        printTask = nullptr;
    }
}

void PrintWorkerHandler::stopTaskGracefully()
{
    if(printTask)
    {
        //If printing was already started, stop it.
        printTask->stop();

        //Set this flag to wait a bit before print task is really stopped
        //This also prevents asking user to Abort a second time
        isStoppingTask = true;

        emit progressChanged(0, tr("Aborting..."));
    }
}
