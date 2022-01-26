#include "printworker.h"
#include "printing/wizard/printwizard.h" //For translations and defs

#include "printing/helper/model/printhelper.h"
#include "printing/helper/model/igraphscenecollection.h"
#include "utils/scene/igraphscene.h"

#include <QPainter>

#include <QPrinter>
#include <QPdfWriter>
#include <QSvgGenerator>

#include "info.h"

#include <QFile>

#include <QtMath>

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
    IQuittableTask(receiver),
    m_printer(nullptr),
    m_collection(nullptr)
{

}

PrintWorker::~PrintWorker()
{

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

void PrintWorker::setCollection(IGraphSceneCollection *newCollection)
{
    m_collection = newCollection;
}

int PrintWorker::getMaxProgress() const
{
    return m_collection->getItemCount() * ProgressStepsForScene;
}

void PrintWorker::setScenePageLay(const Print::PageLayoutOpt &pageLay)
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

    if(!m_collection->startIteration())
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

    while (true)
    {
        if(wasStopped())
        {
            sendEvent(new PrintProgressEvent(this,
                                             PrintProgressEvent::ProgressAbortedByUser,
                                             QString()), true);
            return false;
        }

        const IGraphSceneCollection::SceneItem item = m_collection->getNextItem();
        if(!item.scene)
            break; //Finished

        QScopedPointer<IGraphScene> scenPtr(item.scene);

        const QRectF sourceRect(QPointF(), scenPtr->getContentsSize());

        //Send progress and description
        sendEvent(new PrintProgressEvent(this, progressiveNum * ProgressStepsForScene, item.name), false);

        bool valid = true;
        if(func)
            valid = func(&painter, item.name, sourceRect,
                         item.type, progressiveNum);
        if(!valid)
            return false;

        //Render scene contets
        scenPtr->renderContents(&painter, sourceRect);

        //Render horizontal header
        QRectF horizHeaderRect = sourceRect;
        horizHeaderRect.moveTop(0);
        horizHeaderRect.setBottom(item.scene->getHeaderSize().height());
        scenPtr->renderHeader(&painter, horizHeaderRect, Qt::Horizontal, 0);

        //Render vertical header
        QRectF vertHeaderRect = sourceRect;
        vertHeaderRect.moveLeft(0);
        vertHeaderRect.setRight(item.scene->getHeaderSize().width());
        scenPtr->renderHeader(&painter, vertHeaderRect, Qt::Vertical, 0);

        if(endPaintingEveryPage)
            painter.end();

        progressiveNum++;
    }

    return true;
}

class PrintWorkerProgress : public Print::IProgress
{
public:
    bool reportProgressAndContinue(int current, int max) override
    {
        if(current == Print::IProgress::ProgressSetMaximum)
        {
            //Begin a new scene
            totalPages = max;
            pageNum = 0;
        }
        else
        {
            //Add 1 because curren page is already finished
            pageNum = current + 1;
        }

        //Calculate fraction of current scene
        const double sceneFraction = double(pageNum * PrintWorker::ProgressStepsForScene) / double(totalPages);

        //Calculate progress, completed scenes + fraction of current scene
        const int progress = sceneNumber * PrintWorker::ProgressStepsForScene + qFloor(sceneFraction);

        return m_task->sendProgressOrAbort(progress, name);
    }

public:
    PrintWorker *m_task = nullptr;

    int pageNum = 0;
    int totalPages = 0;
    int sceneNumber = 0;
    QString name;
};

class PagedDevImpl : public Print::IPagedPaintDevice
{
public:
    PagedDevImpl() :m_dev(nullptr) { m_needsInitForEachPage = false; }

    bool newPage(QPainter *painter, const QRectF& brect, bool isFirstPage) override
    {
        if(!isFirstPage)
            return m_dev->newPage();
        return true;
    }
public:
    QPagedPaintDevice *m_dev;
};

bool PrintWorker::printInternalPaged(BeginPaintFunc func, bool endPaintingEveryPage)
{
    PrintWorkerProgress progress;
    progress.m_task = this;
    progress.sceneNumber = 0;

    PagedDevImpl devImpl;

    QPainter painter;

    if(!m_collection->startIteration())
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
    scenePageLay.pageMarginsPenWidthPoints = 3;

    Print::PageNumberOpt pageNumberOpt;
    pageNumberOpt.enable = true;
    pageNumberOpt.fontSizePt = 20;
    pageNumberOpt.font.setBold(true);
    pageNumberOpt.fmt = QStringLiteral("Row: %1/%2 Col: %3/%4");

    Print::PageLayoutScaled scenePageLayScale;
    scenePageLayScale.pageMarginsPen = QPen(Qt::darkRed);

    while (true)
    {
        if(wasStopped())
        {
            sendEvent(new PrintProgressEvent(this,
                                             PrintProgressEvent::ProgressAbortedByUser,
                                             QString()), true);
            return false;
        }

        const IGraphSceneCollection::SceneItem item = m_collection->getNextItem();
        if(!item.scene)
            break; //Finished

        QScopedPointer<IGraphScene> scenPtr(item.scene);

        progress.name = item.name;

        //Send progress and description
        sendEvent(new PrintProgressEvent(this,
                                         progress.sceneNumber * ProgressStepsForScene,
                                         item.name), false);

        const QRectF sceneRect(QPointF(), scenPtr->getContentsSize());
        bool valid = true;
        if(func)
            valid = func(&painter, item.name, sceneRect,
                         item.type, progress.sceneNumber * ProgressStepsForScene);
        if(!valid)
            return false;

        devImpl.m_dev = static_cast<QPagedPaintDevice *>(painter.device());

        //Update printer resolution
        scenePageLayScale.printerResolution = painter.device()->logicalDpiY();
        scenePageLayScale.devicePageRectPixels = QRectF(0, 0, painter.device()->width(), painter.device()->height());
        PrintHelper::initScaledLayout(scenePageLayScale, scenePageLay);

        if(!PrintHelper::printPagedScene(&painter, &devImpl, scenPtr.data(), &progress,
                                          scenePageLayScale, pageNumberOpt))
            return false;

        //Reset transform after evert
        painter.resetTransform();

        if(endPaintingEveryPage)
            painter.end();

        progress.sceneNumber++;
    }

    return true;
}

bool PrintWorker::sendProgressOrAbort(int progress, const QString &msg)
{
    if(wasStopped())
    {
        sendEvent(new PrintProgressEvent(this,
                                         PrintProgressEvent::ProgressAbortedByUser,
                                         QString()), true);
        //Quit
        return false;
    }

    sendEvent(new PrintProgressEvent(this, progress, msg), false);
    return true;
}

bool PrintWorker::printSvg()
{
    std::unique_ptr<QSvgGenerator> svg;
    const QString docTitle = QStringLiteral("Timetable Session (%1)");
    const QString descr = QStringLiteral("Generated by %1").arg(AppDisplayName);

    auto beginPaint = [this, &svg, &docTitle, &descr](QPainter *painter,
                                                      const QString& title, const QRectF& sourceRect,
                                                      const QString& type, int progressiveNum) -> bool
    {
        const QString fileName = Print::getFileName(printOpt.filePath, printOpt.fileNamePattern, QLatin1String(".svg"),
                                                    title, type, progressiveNum);
        svg.reset(new QSvgGenerator);
        svg->setTitle(docTitle.arg(title));
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
    std::unique_ptr<QPdfWriter> writer;

    auto beginPaint = [this, &writer](QPainter *painter,
                                      const QString& title, const QRectF& sourceRect,
                                      const QString& type, int progressiveNum) -> bool
    {
        QString fileName = printOpt.filePath;
        if(printOpt.useOneFileForEachScene)
        {
            //File path is the base directory, build file name
            fileName = Print::getFileName(printOpt.filePath, printOpt.fileNamePattern, QLatin1String(".pdf"),
                                          title, type, progressiveNum);
        }

        if(printOpt.useOneFileForEachScene || progressiveNum == 0)
        {
            //First page of new scene, create a new PDF
            writer.reset(new QPdfWriter(fileName));

            writer->setCreator(AppDisplayName);
            writer->setTitle(title);

            //If page layout is need set it before begin painting
            //When printing each scene on single page we do not set a layout here
            if(!printOpt.printSceneInOnePage)
                writer->setPageLayout(m_printer->pageLayout());
            qDebug() << "LAYOUT:" << writer->pageLayout();
        }

        //If custom page is needed set it before begin painting or adding page
        if(printOpt.printSceneInOnePage)
        {
            //Calculate custom page size (inverse scale factor: Pixel -> Points)
            QSizeF newSize = sourceRect.size() * Print::PrinterDefaultResolution / writer->resolution();
            QPageSize ps(newSize, QPageSize::Point);
            writer->setPageSize(ps);
            writer->setPageMargins(QMarginsF());
        }

        if(printOpt.useOneFileForEachScene || progressiveNum == 0)
        {
            //First page of new scene, begin painting
            if(!painter->begin(writer.get()))
            {
                qWarning() << "PrintWorker::printPdf(): cannot begin QPainter";

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
        }
        else
        {
            //New scene on same file, add page
            if(!writer->newPage())
            {
                qWarning() << "PrintWorker::printPdf(): cannot add page";
                return false;
            }
        }

        if(printOpt.printSceneInOnePage)
        {
            //Scale painter to fix possible custom page size problems
            int wt = writer->width();
            int ht = writer->height();
            const double scaleX = wt / sourceRect.width();
            const double scaleY = ht / sourceRect.height();
            const double scale = qMin(scaleX, scaleY);
            painter->scale(scale, scale);
        }

        return true;
    };

    if(printOpt.printSceneInOnePage)
        return printInternal(beginPaint, printOpt.useOneFileForEachScene);
    return printInternalPaged(beginPaint, printOpt.useOneFileForEachScene);
}

bool PrintWorker::printPaged()
{
    auto beginPaint = [this](QPainter *painter,
                             const QString& title, const QRectF& sourceRect,
                             const QString& type, int progressiveNum) -> bool
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
