#include "printworker.h"
#include "printing/wizard/printwizard.h" //For translations and defs
#include "printing/wizard/sceneselectionmodel.h"

#include "printing/helper/model/printhelper.h"

#include "app/session.h"

#include "graph/model/linegraphscene.h"
#include "graph/view/backgroundhelper.h"

#include <QPainter>

#include <QPrinter>
#include <QPdfWriter>
#include <QSvgGenerator>

#include "info.h"

#include <QFile>

#include <QDebug>

bool testFileIsWriteable(const QString& fileName, QString &errOut)
{
    QFile tmp(fileName);
    const bool existed = tmp.exists();
    bool writable = tmp.open(QFile::WriteOnly);
    errOut = tmp.errorString();

    if(tmp.isOpen())
        tmp.close();
    if(!existed)
        tmp.remove();

    return writable;
}

PrintProgressEvent::PrintProgressEvent(QRunnable *self, int pr, const QString &descrOrErr) :
    QEvent(_Type),
    task(self),
    progress(pr),
    descriptionOrError(descrOrErr)
{

}

PrintWorker::PrintWorker(sqlite3pp::database &db, QObject *receiver) :
    IQuittableTask(receiver)
{
    scene = new LineGraphScene(db);
}

PrintWorker::~PrintWorker()
{
    delete scene;
    scene = nullptr;
}

void PrintWorker::setPrinter(QPrinter *printer)
{
    m_printer = printer;
}

void PrintWorker::setPrintOpt(const Print::PrintBasicOptions &newPrintOpt)
{
    printOpt = newPrintOpt;

    if(printOpt.filePath.endsWith('/'))
        printOpt.filePath.chop(1); //Remove last slash
}

void PrintWorker::setSelection(SceneSelectionModel *newSelection)
{
    selection = newSelection;
}

int PrintWorker::getMaxProgress() const
{
    return selection->getSelectionCount();
}

void PrintWorker::setScenePageLay(const PrintHelper::PageLayoutOpt &pageLay)
{
    scenePageLay = pageLay;
}

void PrintWorker::run()
{
    sendEvent(new PrintProgressEvent(this, 0, QString()), false);

    bool success = true;

    switch (printOpt.outputType)
    {
    case Print::OutputType::Native:
    {
        m_printer->setOutputFormat(QPrinter::NativeFormat);
        success = printPaged();
        break;
    }
    case Print::OutputType::Pdf:
    {
        success = printPdf();
        break;
    }
    case Print::OutputType::Svg:
    {
        success = printSvg();
        break;
    }
    case Print::OutputType::NTypes:
        break;
    }

    if(success)
    {
        //Send 'Finished' and quit
        sendEvent(new PrintProgressEvent(this,
                                         PrintProgressEvent::ProgressMaxFinished,
                                         QString()), true);
    }
}

bool PrintWorker::printInternal(BeginPaintFunc func, bool endPaintingEveryPage)
{
    QPainter painter;

    if(!selection->startIteration())
    {
        //Send error and quit
        sendEvent(new PrintProgressEvent(this,
                                         PrintProgressEvent::ProgressError,
                                         PrintWizard::tr("Cannot iterate items.\n"
                                                         "Check database connection.")),
                  true);
        return false;
    }

    int progressiveNum = 0;

    const int vertOffset = Session->vertOffset;
    const int horizOffset = Session->horizOffset;

    while (true)
    {
        if(wasStopped())
        {
            sendEvent(new PrintProgressEvent(this,
                                             PrintProgressEvent::ProgressAbortedByUser,
                                             QString()), true);
            return false;
        }

        const SceneSelectionModel::Entry entry = selection->getNextEntry();
        if(!entry.objectId)
            break; //Finished

        if(!scene->loadGraph(entry.objectId, entry.type))
            continue; //Loading error, skip

        const QRectF sourceRect(QPointF(), scene->getContentsSize());

        //Send progress and description
        sendEvent(new PrintProgressEvent(this, progressiveNum, scene->getGraphObjectName()), false);

        bool valid = true;
        if(func)
            valid = func(&painter, scene->getGraphObjectName(), sourceRect,
                         entry.type, progressiveNum);
        if(!valid)
            return false;

        QRectF stationLabelRect = sourceRect;
        stationLabelRect.setHeight(vertOffset - 5); //See LineGraphView::resizeHeaders()

        QRectF hourPanelRect = sourceRect;
        hourPanelRect.setWidth(horizOffset - 5); //See LineGraphView::resizeHeaders()

        BackgroundHelper::drawBackgroundHourLines(&painter, sourceRect);
        BackgroundHelper::drawStations(&painter, scene, sourceRect);
        BackgroundHelper::drawJobStops(&painter, scene, sourceRect, false);
        BackgroundHelper::drawJobSegments(&painter, scene, sourceRect, false);
        BackgroundHelper::drawStationHeader(&painter, scene, stationLabelRect);
        BackgroundHelper::drawHourPanel(&painter, hourPanelRect);

        if(endPaintingEveryPage)
            painter.end();

        progressiveNum++;
    }

    return true;
}

class PrintWorkerProgress : public PrintHelper::IProgress
{
public:
    bool reportProgressAndContinue(int current, int max) override
    {
        //Cumulate progress with previous scenes FIXME: better logic
        int totalMax = (progressiveNum + 5) * 10;
        int progress = progressiveNum * 10 + (current * 10 / max);
        return m_task->sendProgressOrAbort(progress, totalMax,
                                           m_scene ? m_scene->getGraphObjectName() : QString());
    }

public:
    PrintWorker *m_task = nullptr;

    LineGraphScene *m_scene = nullptr;
    int progressiveNum = 0;
};

class PrinterImpl : public PrintHelper::IPagedPaintDevice
{
public:
    PrinterImpl() :m_printer(nullptr) { m_needsInitForEachPage = false; }

    bool newPage(QPainter *painter, const QRectF& brect, bool isFirstPage) override
    {
        if(!isFirstPage)
            return m_printer->newPage();
        return true;
    }
public:
    QPrinter *m_printer;
};

bool PrintWorker::printInternalPaged(BeginPaintFunc func, bool endPaintingEveryPage)
{
    PrintWorkerProgress progress;
    progress.m_task = this;
    progress.m_scene = nullptr;
    progress.progressiveNum = 0;

    PrinterImpl devImpl;
    devImpl.m_printer = m_printer;

    QPainter painter;

    if(!selection->startIteration())
    {
        //Send error and quit
        sendEvent(new PrintProgressEvent(this,
                                         PrintProgressEvent::ProgressError,
                                         PrintWizard::tr("Cannot iterate items.\n"
                                                         "Check database connection.")),
                  true);
        return false;
    }

    scenePageLay.drawPageMargins = true;
    scenePageLay.pageMarginsPenWidth = 5;
    scenePageLay.pageMarginsPen = QPen(Qt::darkRed, scenePageLay.pageMarginsPenWidth);

    //Calculate members
    scenePageLay.devicePageRect = QRectF(QPointF(), QSizeF(m_printer->width(), m_printer->height()));

    PrintHelper::PageLayoutScaled scenePageLayScale;
    scenePageLayScale.lay = scenePageLay;
    scenePageLayScale.scaledPageRect = QRectF(scenePageLay.devicePageRect.topLeft(),
                                              scenePageLay.devicePageRect.size() / scenePageLay.scaleFactor);
    scenePageLayScale.overlapMarginWidthScaled = scenePageLay.marginOriginalWidth / scenePageLay.scaleFactor;
    scenePageLayScale.isFirstPage = true;

    PrintHelper::PageNumberOpt pageNumberOpt;
    pageNumberOpt.enable = true;
    pageNumberOpt.fontSize = 20;
    pageNumberOpt.font.setBold(true);
    pageNumberOpt.fmt = QStringLiteral("Row: %1/%2 Col: %3/%4");

    while (true)
    {
        if(wasStopped())
        {
            sendEvent(new PrintProgressEvent(this,
                                             PrintProgressEvent::ProgressAbortedByUser,
                                             QString()), true);
            return false;
        }

        const SceneSelectionModel::Entry entry = selection->getNextEntry();
        if(!entry.objectId)
            break; //Finished

        if(!scene->loadGraph(entry.objectId, entry.type))
            continue; //Loading error, skip

        progress.m_scene = scene;

        //Send progress and description
        sendEvent(new PrintProgressEvent(this,
                                         progress.progressiveNum,
                                         scene->getGraphObjectName()), false);


        const QRectF sceneRect(QPointF(), scene->getContentsSize());
        bool valid = true;
        if(func)
            valid = func(&painter, scene->getGraphObjectName(), sceneRect,
                         entry.type, progress.progressiveNum);
        if(!valid)
            return false;

        PrintHelper::printPagedScene(&painter, &devImpl, scene, &progress,
                                     scenePageLayScale, pageNumberOpt);

        if(endPaintingEveryPage)
            painter.end();

        progress.progressiveNum++;
    }

    return true;
}

bool PrintWorker::sendProgressOrAbort(int cur, int max, const QString &msg)
{
    if(wasStopped())
    {
        sendEvent(new PrintProgressEvent(this,
                                         PrintProgressEvent::ProgressAbortedByUser,
                                         QString()), true);
        //Quit
        return false;
    }

    //Send progress and description FIXME: pass max?
    sendEvent(new PrintProgressEvent(this, cur, msg), false);
    return true;
}

bool PrintWorker::printSvg()
{
    std::unique_ptr<QSvgGenerator> svg;
    const QString docTitle = QStringLiteral("Timetable Session");
    const QString descr = QStringLiteral("Generated by %1").arg(AppDisplayName);

    auto beginPaint = [this, &svg, &docTitle, &descr](QPainter *painter,
                                                      const QString& title, const QRectF& sourceRect,
                                                      LineGraphType type, int progressiveNum) -> bool
    {
        const QString fileName = Print::getFileName(printOpt.filePath, printOpt.fileNamePattern, QLatin1String(".svg"),
                                                    title, type, progressiveNum);
        svg.reset(new QSvgGenerator);
        svg->setTitle(docTitle);
        svg->setDescription(descr);
        svg->setFileName(fileName);
        svg->setSize(sourceRect.size().toSize());
        svg->setViewBox(sourceRect);

        if(!painter->begin(svg.get()))
        {
            qWarning() << "PrintWorker::printSvg(): cannot begin QPainter";
            QString fileErr;
            bool writable = testFileIsWriteable(fileName, fileErr);

            QString msg;
            if(!writable)
            {
                msg = PrintWizard::tr("SVG Error: cannot open output file.\n"
                         "Path: \"%1\"\n"
                         "Error: %2").arg(fileName, fileErr);
            }
            else
            {
                msg = PrintWizard::tr("SVG Error: generic error.");
            }

            //Send error and quit
            sendEvent(new PrintProgressEvent(this,
                                             PrintProgressEvent::ProgressError,
                                             msg), true);
            return false;
        }

        return true;
    };

    return printInternal(beginPaint, true);
}

bool PrintWorker::printPdf()
{
    m_printer->setOutputFormat(QPrinter::PdfFormat);
    m_printer->setCreator(AppDisplayName);
    m_printer->setDocName(QStringLiteral("Timetable Session"));

    if(printOpt.useOneFileForEachScene)
    {
        return printPdfMultipleFiles();
    }
    else
    {
        m_printer->setOutputFileName(printOpt.filePath);
        return printPaged();
    }
}

bool PrintWorker::printPdfMultipleFiles()
{
    std::unique_ptr<QPdfWriter> writer;

    auto beginPaint = [this, &writer](QPainter *painter,
                                      const QString& title, const QRectF& sourceRect,
                                      LineGraphType type, int progressiveNum) -> bool
    {
        const QString fileName = Print::getFileName(printOpt.filePath, printOpt.fileNamePattern, QLatin1String(".pdf"),
                                                    title, type, progressiveNum);
        writer.reset(new QPdfWriter(fileName));
        QPageSize ps(sourceRect.size(), QPageSize::Point);
        writer->setPageSize(ps);


        if(!painter->begin(writer.get()))
        {
            qWarning() << "PrintWorker::printPdfMultipleFiles(): cannot begin QPainter";

            QString fileErr;
            bool writable = testFileIsWriteable(fileName, fileErr);

            QString msg;
            if(!writable)
            {
                msg = PrintWizard::tr("PDF Error: cannot open output file.\n"
                         "Path: \"%1\"\n"
                         "Error: %2").arg(fileName, fileErr);
            }
            else
            {
                msg = PrintWizard::tr("PDF Error: generic error.");
            }

            //Send error and quit
            sendEvent(new PrintProgressEvent(this,
                                             PrintProgressEvent::ProgressError,
                                             msg), true);
            return false;
        }

        const double scaleX = writer->width() / sourceRect.width();
        const double scaleY = writer->height() / sourceRect.height();
        const double scale = qMin(scaleX, scaleY);
        painter->scale(scale, scale);

        return true;
    };

    return printInternal(beginPaint, true);
}

bool PrintWorker::printPaged()
{
    auto beginPaint = [this](QPainter *painter,
                             const QString& title, const QRectF& sourceRect,
                             LineGraphType type, int progressiveNum) -> bool
    {
        bool success = true;
        QString errMsg;

        if(progressiveNum == 0)
        {
            //First page
            if(!painter->begin(m_printer))
            {
                qWarning() << "PrintWorker::printPaged(): cannot begin QPainter";
                errMsg = PrintWizard::tr("Cannot begin painting");
                success = false;
            }
        }
        else
        {
            if(!m_printer->newPage())
            {
                qWarning() << "PrintWorker::printPaged(): cannot add new page";
                errMsg = PrintWizard::tr("Cannot create new page");
                success = false;
            }
        }

        if(!success)
        {
            //Send error and quit
            sendEvent(new PrintProgressEvent(this,
                                             PrintProgressEvent::ProgressError,
                                             errMsg), true);
            return false;
        }

        return true;
    };

    return printInternalPaged(beginPaint, false);
}
