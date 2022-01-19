#ifndef PRINTWORKER_H
#define PRINTWORKER_H

#include "utils/thread/iquittabletask.h"
#include "utils/worker_event_types.h"

#include <QVector>

#include "printdefs.h"

#include "utils/types.h"

#include <QRectF>
#include <QPen>
#include <QFont>

class QPrinter;
class QPainter;
class QRectF;

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

    void setOutputType(Print::OutputType type);
    void setFileOutput(const QString &value, bool different);
    void setFilePattern(const QString &newFilePatter);

    void setSelection(SceneSelectionModel *newSelection);
    int getMaxProgress() const;

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
    SceneSelectionModel *selection;

    QString fileOutput;
    QString filePattern;
    bool differentFiles;
    Print::OutputType outType;

    LineGraphScene *scene;
};

class PrintHelper
{
public:
    //Page Layout
    struct PageLayoutOpt
    {
        QRectF devicePageRect;
        QRectF scaledPageRect;

        double scaleFactor;
        double overlapMarginWidth;

        int horizPageCnt;
        int vertPageCnt;

        bool drawPageMargins;
        double pageMarginsPenWidth;
        QPen pageMarginsPen;

        bool isFirstPage;
    };

    //Page Numbers
    struct PageNumberOpt
    {
        QFont font;
        QString fmt;
        double fontSize;
        bool enable;
    };

    //Device
    class IPagedPaintDevice
    {
    public:
        virtual ~IPagedPaintDevice() {};
        virtual bool newPage(QPainter *painter, bool isFirstPage) = 0;

        inline bool needsInitForEachPage() const { return m_needsInitForEachPage; }

    protected:
        bool m_needsInitForEachPage;
    };

    class IRenderScene
    {
    public:
        virtual ~IRenderScene() {};
        virtual bool render(QPainter *painter, const QRectF& sceneRect) = 0;

        inline QSizeF getContentSize() const { return m_contentSize; }

    protected:
        QSizeF m_contentSize;
    };

    class IProgress
    {
    public:
        virtual ~IProgress() {};
        virtual bool reportProgressAndContinue(int current, int max) = 0;
    };

    bool printPagedScene(QPainter *painter, IPagedPaintDevice *dev, IRenderScene *scene, IProgress *progress,
                         PageLayoutOpt& pageLay, PageNumberOpt& pageNumOpt);
};

#endif // PRINTWORKER_H
