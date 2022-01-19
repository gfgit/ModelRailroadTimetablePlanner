#include "printworker.h"
#include "printwizard.h" //For translations and defs

#include "app/session.h"

#include "sceneselectionmodel.h"

#include "graph/model/linegraphscene.h"
#include "graph/view/backgroundhelper.h"

#include <QPainter>

#include <QPrinter>
#include <QPdfWriter>
#include <QSvgGenerator>

#include <QtMath>

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
    IQuittableTask(receiver),
    differentFiles(false),
    outType(Print::Native)
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

void PrintWorker::setOutputType(Print::OutputType type)
{
    outType = type;
}

void PrintWorker::setFileOutput(const QString &value, bool different)
{
    fileOutput = value;
    if(fileOutput.endsWith('/'))
        fileOutput.chop(1);
    differentFiles = different;
}

void PrintWorker::setFilePattern(const QString &newFilePatter)
{
    filePattern = newFilePatter;
}

void PrintWorker::setSelection(SceneSelectionModel *newSelection)
{
    selection = newSelection;
}

int PrintWorker::getMaxProgress() const
{
    return selection->getSelectionCount();
}

void PrintWorker::run()
{
    sendEvent(new PrintProgressEvent(this, 0, QString()), false);

    bool success = true;

    switch (outType)
    {
    case Print::Native:
    {
        m_printer->setOutputFormat(QPrinter::NativeFormat);
        success = printPaged();
        break;
    }
    case Print::Pdf:
    {
        success = printPdf();
        break;
    }
    case Print::Svg:
    {
        success = printSvg();
        break;
    }
    case Print::NTypes:
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

        const QRectF sourceRect(QPointF(), scene->getContentSize());

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
        BackgroundHelper::drawStationHeader(&painter, scene, stationLabelRect, 0);
        BackgroundHelper::drawHourPanel(&painter, hourPanelRect, 0);

        if(endPaintingEveryPage)
            painter.end();

        progressiveNum++;
    }

    return true;
}

bool PrintWorker::printInternalPaged(BeginPaintFunc func, bool endPaintingEveryPage)
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

    //To avoid calling newPage() at end of loop
    //which would result in an empty extra page after last drawn page
    bool isFirstPage = true;

    bool drawPageFrame = true;
    double pageFramePenWidth = 5;
    QPen pageFramePen(Qt::darkRed, pageFramePenWidth);

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

        const QRectF sceneRect(QPointF(), scene->getContentSize());

        //Send progress and description
        sendEvent(new PrintProgressEvent(this, progressiveNum, scene->getGraphObjectName()), false);

        bool valid = true;
        if(func)
            valid = func(&painter, scene->getGraphObjectName(), sceneRect,
                         entry.type, progressiveNum);
        if(!valid)
            return false;

        //Reset transformation on new scene
        painter.resetTransform();

        QPagedPaintDevice *dev = static_cast<QPagedPaintDevice *>(painter.device());
        const QRectF deviceRect(QPointF(), QSizeF(dev->width(), dev->height()));

        //Apply scaling
        qreal scaleFactor = 5; //FIXME: arbitrary value for testing, make user option
        QRectF targetRect(deviceRect.topLeft(), deviceRect.size() / scaleFactor);
        painter.scale(scaleFactor, scaleFactor);

        //Inverse scale frame pen to keep it independent
        pageFramePen.setWidth(pageFramePenWidth / scaleFactor);

        //Apply overlap margin
        qreal overlapMargin = targetRect.width() / 15; //FIXME: arbitrary value for testing, make user option

        //Each page has 2 margin (left and right) and other 2 margins (top and bottom)
        //Calculate effective content size
        QPointF effectivePageOrigin(overlapMargin, overlapMargin);
        QSizeF effectivePageSize = targetRect.size();
        effectivePageSize.rwidth() -= 2 * overlapMargin;
        effectivePageSize.rheight() -= 2 * overlapMargin;

        int horizPageCount = qCeil(sceneRect.width() / effectivePageSize.width());
        int vertPageCount = qCeil(sceneRect.height() / effectivePageSize.height());

        QRectF stationLabelRect = sceneRect;
        stationLabelRect.setHeight(vertOffset - 5); //See LineGraphView::resizeHeaders()

        QRectF hourPanelRect = sceneRect;
        hourPanelRect.setWidth(horizOffset - 5); //See LineGraphView::resizeHeaders()

        QRectF sourceRect = targetRect;
        stationLabelRect.setWidth(sourceRect.width());
        hourPanelRect.setHeight(sourceRect.height());

        //FIXME: remove, temp fix to align correctly in print preview dialog
        dev->newPage(); //This skips 'Cover' page when chosing booklet 2 page layout

        for(int y = 0; y < vertPageCount; y++)
        {
            //Start at leftmost column
            painter.translate(sourceRect.left(), 0);
            sourceRect.moveLeft(0);

            for(int x = 0; x < horizPageCount; x++)
            {
                if(isFirstPage)
                    isFirstPage = false;
                else
                    dev->newPage();

                //Clipping must be set on every new page
                //Since clipping works in logical (QPainter) coordinates
                //we use sourceRect which is already transformed
                painter.setClipRect(sourceRect);

                BackgroundHelper::drawBackgroundHourLines(&painter, sourceRect);
                BackgroundHelper::drawStations(&painter, scene, sourceRect);
                BackgroundHelper::drawJobStops(&painter, scene, sourceRect, false);
                BackgroundHelper::drawJobSegments(&painter, scene, sourceRect, false);

                if(x == 0)
                {
                    //Left column pages have hour labels
                    hourPanelRect.moveTop(sourceRect.top());
                    BackgroundHelper::drawHourPanel(&painter, hourPanelRect, 0);
                }

                if(y == 0)
                {
                    //Top row pages have station labels
                    stationLabelRect.moveLeft(sourceRect.left());
                    BackgroundHelper::drawStationHeader(&painter, scene, stationLabelRect, 0);
                }

                if(drawPageFrame)
                {
                    //Draw a frame to help align printed pages
                    painter.setPen(pageFramePen);
                    QLineF arr[4] = {
                        //Top
                        QLineF(sourceRect.left(),  sourceRect.top() + overlapMargin,
                               sourceRect.right(), sourceRect.top() + overlapMargin),
                        //Bottom
                        QLineF(sourceRect.left(),  sourceRect.bottom() - overlapMargin,
                               sourceRect.right(), sourceRect.bottom() - overlapMargin),
                        //Left
                        QLineF(sourceRect.left() + overlapMargin, sourceRect.top(),
                               sourceRect.left() + overlapMargin, sourceRect.bottom()),
                        //Right
                        QLineF(sourceRect.right() - overlapMargin, sourceRect.top(),
                               sourceRect.right() - overlapMargin, sourceRect.bottom())
                    };
                    painter.drawLines(arr, 4);
                }

                //Go left
                sourceRect.moveLeft(sourceRect.left() + effectivePageSize.width());
                painter.translate(-effectivePageSize.width(), 0);
            }

            //Go down and back to left most column
            sourceRect.moveTop(sourceRect.top() + effectivePageSize.height());
            painter.translate(sourceRect.left(), -effectivePageSize.height());
            sourceRect.moveLeft(0);
        }

        //Reset to top most and left most
        painter.translate(sourceRect.topLeft());
        sourceRect.moveTopLeft(QPointF());

        if(endPaintingEveryPage)
            painter.end();

        progressiveNum++;
    }

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
        const QString fileName = Print::getFileName(fileOutput, filePattern, QLatin1String(".svg"),
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

    if(differentFiles)
    {
        return printPdfMultipleFiles();
    }
    else
    {
        m_printer->setOutputFileName(fileOutput);
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
        const QString fileName = Print::getFileName(fileOutput, filePattern, QLatin1String(".pdf"),
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
