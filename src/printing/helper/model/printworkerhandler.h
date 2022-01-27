#ifndef PRINTWORKERHANDLER_H
#define PRINTWORKERHANDLER_H

#include <QObject>

#include "printdefs.h"
#include "printing/helper/model/printhelper.h"

class PrintWorker;
class IGraphSceneCollection;

namespace sqlite3pp {
class database;
}

class PrintWorkerHandler : public QObject
{
    Q_OBJECT
public:
    explicit PrintWorkerHandler(sqlite3pp::database &db, QObject *parent = nullptr);

    inline Print::PrintBasicOptions getOptions() const { return printOpt; }
    void setOptions(const Print::PrintBasicOptions& opt, QPrinter *printer);

    inline Print::PageLayoutOpt getScenePageLay() const { return scenePageLay; }
    inline void setScenePageLay(const Print::PageLayoutOpt& lay) { scenePageLay = lay; }

    bool event(QEvent *e) override;

    inline bool taskIsRunning() const { return printTask != nullptr; }
    inline bool waitingForTaskToStop() const { return isStoppingTask; }

signals:
    void progressMaxChanged(int max);
    void progressChanged(int val, const QString& msg);
    void progressFinished(bool success, const QString& msg);

public:
    void startPrintTask(QPrinter *printer, IGraphSceneCollection *collection);
    void abortPrintTask();
    void stopTaskGracefully();

private:
    sqlite3pp::database &mDb;

    PrintWorker *printTask;

    Print::PrintBasicOptions printOpt;
    Print::PageLayoutOpt scenePageLay;

    bool isStoppingTask;
};

#endif // PRINTWORKERHANDLER_H
