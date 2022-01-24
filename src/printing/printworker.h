#ifndef PRINTWORKER_H
#define PRINTWORKER_H

#include "utils/thread/iquittabletask.h"
#include "utils/worker_event_types.h"

#include <QVector>

#include "printdefs.h"

#include "utils/types.h"

#include "printing/helper/model/printhelper.h"

class QPrinter;
class QPainter;

class SceneSelectionModel;

class LineGraphScene;

namespace sqlite3pp {
class database;
}

class PrintProgressEvent : public QEvent
{
public:
    enum
    {
        ProgressError = -1,
        ProgressAbortedByUser = -2,
        ProgressMaxFinished = -3
    };

    static constexpr Type _Type = Type(CustomEvents::PrintProgress);

    PrintProgressEvent(QRunnable *self, int pr, const QString& descrOrErr);

public:
    QRunnable *task;
    int progress;
    QString descriptionOrError;
};


class PrintWorker : public IQuittableTask
{
public:
    PrintWorker(sqlite3pp::database &db, QObject *receiver);
    ~PrintWorker();

    void setPrinter(QPrinter *printer);

    inline Print::PrintBasicOptions getPrintOpt() const { return printOpt; };
    void setPrintOpt(const Print::PrintBasicOptions &newPrintOpt);

    void setSelection(SceneSelectionModel *newSelection);
    int getMaxProgress() const;

    void setScenePageLay(const PrintHelper::PageLayoutOpt& pageLay);

    //IQuittableTask
    void run() override;

private:
    bool printSvg();
    bool printPdf();
    bool printPdfMultipleFiles();
    bool printPaged();

private:
    typedef std::function<bool(QPainter *painter,
                               const QString& title, const QRectF& sourceRect,
                               LineGraphType type, int progressiveNum)> BeginPaintFunc;

    bool printInternal(BeginPaintFunc func, bool endPaintingEveryPage);
    bool printInternalPaged(BeginPaintFunc func, bool endPaintingEveryPage);

public:
    bool sendProgressOrAbort(int cur, int max, const QString& msg);

private:
    QPrinter *m_printer;
    Print::PrintBasicOptions printOpt;
    PrintHelper::PageLayoutOpt scenePageLay;

    SceneSelectionModel *selection;

    LineGraphScene *scene;
};

#endif // PRINTWORKER_H
