#include "printpreviewsceneproxy.h"

#include <QPainter>

#include <QtMath>

PrintPreviewSceneProxy::PrintPreviewSceneProxy(QObject *parent) :
    IGraphScene(parent),
    sourceScene(nullptr),
    viewScaleFactor(1)
{
    originalHeaderSize = QSizeF(70, 70);
    setViewScaleFactor(1.0);

    //Make margin pen a bit bigger
    pageLay.pageMarginsPenWidth = 7;
    updatePageLay();
}

void PrintPreviewSceneProxy::renderContents(QPainter *painter, const QRectF &sceneRect)
{
    const QTransform originalTransform = painter->worldTransform();

    if(sourceScene)
    {
        //Map coordinates to source scene
        QRectF sourceRect = sceneRect;

        //Shift contents by our headers size and top left page margin
        QPointF origin(m_cachedHeaderSize.width() + pageLay.marginOriginalWidth,
                       m_cachedHeaderSize.height() + pageLay.marginOriginalWidth);
        sourceRect.moveTopLeft(sourceRect.topLeft() - origin);

        //Apply scale factor
        sourceRect = QRectF(sourceRect.topLeft() / pageLay.scaleFactor,
                            sourceRect.size() / pageLay.scaleFactor);

        //Cut negative part which would go under our headers
        //This will reduce a bit the rect size
        if(sourceRect.left() < 0)
            sourceRect.setLeft(0);
        if(sourceRect.top() < 0)
            sourceRect.setTop(0);

        //Cut possible extra parts
        QSizeF contentsSize = sourceScene->getContentsSize();
        if(sourceRect.right() > contentsSize.width())
            sourceRect.setRight(contentsSize.width());
        if(sourceRect.bottom() > contentsSize.height())
            sourceRect.setBottom(contentsSize.height());

        painter->translate(origin);
        painter->scale(pageLay.scaleFactor, pageLay.scaleFactor);

        //Draw source contents
        sourceScene->renderContents(painter, sourceRect);

        QSizeF headerSize = sourceScene->getHeaderSize();

        //If on top draw horizontal header
        if(sourceRect.top() < headerSize.height())
        {
            QRectF horizHeaderRect = sourceRect;
            horizHeaderRect.setBottom(headerSize.height());
            sourceScene->renderHeader(painter, horizHeaderRect, Qt::Horizontal, 0);
        }

        //If on left draw vertical header
        if(sourceRect.left() < headerSize.width())
        {
            QRectF vertHeaderRect = sourceRect;
            vertHeaderRect.setRight(headerSize.width());
            sourceScene->renderHeader(painter, vertHeaderRect, Qt::Vertical, 0);
        }

        //Wash out a bit to make page borders more visible
        painter->fillRect(sourceRect, QColor(0, 255, 0, 80));
    }

    //Reset transorm to previous
    painter->setWorldTransform(originalTransform);

    //TODO: draw page borders and margins on top of scene
    drawPageBorders(painter, sceneRect, false);
}

void PrintPreviewSceneProxy::renderHeader(QPainter *painter, const QRectF &sceneRect,
                                          Qt::Orientation orient, double scroll)
{
    double vertScroll = scroll;
    double horizScroll = 0;
    if(orient == Qt::Horizontal)
        qSwap(vertScroll, horizScroll);

    //Do not overlap the 2 headers in the top left corner for background and separator lines
    QRectF nonOverlappingingRect = sceneRect;
    if(nonOverlappingingRect.left() < m_cachedHeaderSize.width() + horizScroll)
        nonOverlappingingRect.setLeft(m_cachedHeaderSize.width() + horizScroll);

    if(nonOverlappingingRect.top() < m_cachedHeaderSize.width() + vertScroll)
        nonOverlappingingRect.setTop(m_cachedHeaderSize.width() + vertScroll);

    //Draw a ligth gray background to tell it's a header
    QRectF backGroundRect = sceneRect;
    if(orient == Qt::Vertical)
    {
        //Do not draw corner, it's already drawn by horizontal header
        backGroundRect = nonOverlappingingRect;
        backGroundRect.setLeft(sceneRect.left()); //Keep original left edge
    }
    painter->fillRect(backGroundRect, QColor(101, 101, 101, 128));

    //Draw a separation line of fixed size
    QPen separatorPen(Qt::darkGreen, 5.0 / viewScaleFactor, Qt::SolidLine, Qt::FlatCap);
    painter->setPen(separatorPen);

    if(orient == Qt::Horizontal)
    {
        //Horizontal line
        painter->drawLine(QLineF(sceneRect.left(), m_cachedHeaderSize.height(),
                                 nonOverlappingingRect.right(), m_cachedHeaderSize.height()));
        //Vertical line
        painter->drawLine(QLineF(m_cachedHeaderSize.width() + horizScroll, sceneRect.top(),
                                 m_cachedHeaderSize.width() + horizScroll, nonOverlappingingRect.bottom()));
    }
    else
    {
        //Horizontal line
        painter->drawLine(QLineF(sceneRect.left(), m_cachedHeaderSize.height() + vertScroll,
                                 nonOverlappingingRect.right(), m_cachedHeaderSize.height() + vertScroll));
        //Vertical line
        painter->drawLine(QLineF(m_cachedHeaderSize.width(), nonOverlappingingRect.top(),
                                 m_cachedHeaderSize.width(), nonOverlappingingRect.bottom()));
    }


    //TODO: draw page number header
    drawPageBorders(painter, sceneRect, true, orient);
}

IGraphScene *PrintPreviewSceneProxy::getSourceScene() const
{
    return sourceScene;
}

void PrintPreviewSceneProxy::setSourceScene(IGraphScene *newSourceScene)
{
    //Connect to size and content changes to forward them to view
    //Since this is a print preview we are not interested
    //in sceneActivated() and requestShowRect() signals

    if(sourceScene)
    {
        disconnect(sourceScene, &QObject::destroyed, this,
                   &PrintPreviewSceneProxy::onSourceSceneDestroyed);
        disconnect(sourceScene, &IGraphScene::redrawGraph,
                   this, &PrintPreviewSceneProxy::updateSourceSizeAndRedraw);
        disconnect(sourceScene, &IGraphScene::headersSizeChanged,
                   this, &PrintPreviewSceneProxy::updateSourceSizeAndRedraw);
    }
    sourceScene = newSourceScene;
    if(sourceScene)
    {
        connect(sourceScene, &QObject::destroyed, this,
                &PrintPreviewSceneProxy::onSourceSceneDestroyed);
        connect(sourceScene, &IGraphScene::redrawGraph,
                this, &PrintPreviewSceneProxy::updateSourceSizeAndRedraw);
        connect(sourceScene, &IGraphScene::headersSizeChanged,
                this, &PrintPreviewSceneProxy::updateSourceSizeAndRedraw);
    }

    updateSourceSizeAndRedraw();
}

double PrintPreviewSceneProxy::getViewScaleFactor() const
{
    return viewScaleFactor;
}

void PrintPreviewSceneProxy::setViewScaleFactor(double newViewScaleFactor)
{
    //Keep header of same size, independently of view zoom
    viewScaleFactor = newViewScaleFactor;

    const QSizeF newHeaderSize = originalHeaderSize / viewScaleFactor;
    if(newHeaderSize != m_cachedHeaderSize)
    {
        m_cachedHeaderSize = newHeaderSize;
        emit headersSizeChanged();
        updateSourceSizeAndRedraw();
    }
}

QRectF PrintPreviewSceneProxy::getPageSize() const
{
    return pageLay.devicePageRect;
}

void PrintPreviewSceneProxy::setPageSize(const QRectF &newPageSize)
{
    pageLay.devicePageRect = newPageSize;
    updatePageLay();
}

double PrintPreviewSceneProxy::getSourceScaleFactor() const
{
    return pageLay.scaleFactor;
}

void PrintPreviewSceneProxy::setSourceScaleFactor(double newSourceScaleFactor)
{
    pageLay.scaleFactor = newSourceScaleFactor;
    updatePageLay();
}

void PrintPreviewSceneProxy::onSourceSceneDestroyed()
{
    sourceScene = nullptr;
    updateSourceSizeAndRedraw();
}

void PrintPreviewSceneProxy::updateSourceSizeAndRedraw()
{
    QSizeF srcContentsSize;
    if(sourceScene)
        srcContentsSize = sourceScene->getContentsSize() * pageLay.scaleFactor;

    //NOTE: do not shift by top left margin here
    //This is because it should not be counted when calculating page count
    //as it is out of effective page size (= page inside margins)
    //Overall size is then computed from page count so it's always correct

    //Apply overlap margin
    //Each page has 2 margin (left and right) and other 2 margins (top and bottom)
    //Calculate effective content size inside margins
    effectivePageSize = pageLay.devicePageRect.size();
    effectivePageSize.rwidth() -= 2 * pageLay.marginOriginalWidth;
    effectivePageSize.rheight() -= 2 * pageLay.marginOriginalWidth;

    //Calculate page count, at least 1 page
    pageLay.horizPageCnt = qMax(1, qCeil(srcContentsSize.width() / effectivePageSize.width()));
    pageLay.vertPageCnt = qMax(1, qCeil(srcContentsSize.height() / effectivePageSize.height()));

    QSizeF printedSize(pageLay.horizPageCnt * pageLay.devicePageRect.width(),
                       pageLay.vertPageCnt * pageLay.devicePageRect.height());

    //Adjust by substracting overlapped edges ((2 * n) - 2)
    printedSize -= QSizeF((2 * pageLay.horizPageCnt - 2) * pageLay.marginOriginalWidth,
                          (2 * pageLay.vertPageCnt - 2)* pageLay.marginOriginalWidth);

    //Add our headers
    m_cachedContentsSize = printedSize + m_cachedHeaderSize;

    emit redrawGraph();
}

void PrintPreviewSceneProxy::updatePageLay()
{
    pageLay.scaledPageRect = QRectF(pageLay.devicePageRect.topLeft(), pageLay.devicePageRect.size() / pageLay.scaleFactor);
    pageLay.overlapMarginWidthScaled = pageLay.marginOriginalWidth / pageLay.scaleFactor;
    pageLay.pageMarginsPen.setWidthF(pageLay.pageMarginsPenWidth / pageLay.scaleFactor);

    updateSourceSizeAndRedraw();
}

void PrintPreviewSceneProxy::drawPageBorders(QPainter *painter, const QRectF &sceneRect, bool isHeader, Qt::Orientation orient)
{
    const qreal fixedOffsetX = m_cachedHeaderSize.width() + pageLay.marginOriginalWidth;
    const qreal fixedOffsetY = m_cachedHeaderSize.height() + pageLay.marginOriginalWidth;

    //Theese are effective page sizes (so inside margins)
    int firstPageVertBorder = qCeil((sceneRect.left() - fixedOffsetX) / effectivePageSize.width());
    int lastPageVertBorder = qFloor((sceneRect.right() - fixedOffsetX) / effectivePageSize.width());

    //For N pages there are N + 1 borders, but we count from 0 so last border is N

    //Vertical border, horizontal page count
    if(lastPageVertBorder > pageLay.horizPageCnt)
        lastPageVertBorder = pageLay.horizPageCnt;
    if(firstPageVertBorder > lastPageVertBorder)
        firstPageVertBorder = lastPageVertBorder;
    if(firstPageVertBorder < 0)
        firstPageVertBorder = 0;

    int firstPageHorizBorder = qCeil((sceneRect.top() - fixedOffsetY) / effectivePageSize.height());
    int lastPageHorizBorder = qFloor((sceneRect.bottom() - fixedOffsetY) / effectivePageSize.height());

    //Horizontal border, vertical page count
    if(lastPageHorizBorder > pageLay.vertPageCnt)
        lastPageHorizBorder = pageLay.vertPageCnt;
    if(firstPageHorizBorder > lastPageHorizBorder)
        firstPageHorizBorder = lastPageHorizBorder;
    if(firstPageHorizBorder < 0)
        firstPageHorizBorder = 0;

    qreal pageBordersLeft = sceneRect.left();
    qreal pageBordersTop = sceneRect.top();

    if(!isHeader)
    {
        //Do not go under headers when drawing contents
        pageBordersLeft = qMax(m_cachedHeaderSize.width(), sceneRect.left());
        pageBordersTop = qMax(m_cachedHeaderSize.height(), sceneRect.top());
    }


    //Do not exceed scene proxy size
    const qreal pageBordersRight = qMin(m_cachedContentsSize.width(), sceneRect.right());
    const qreal pageBordersBottom = qMin(m_cachedContentsSize.height(), sceneRect.bottom());

    //Draw effective page borders
    QPen borderPen(Qt::red, pageLay.pageMarginsPenWidth, Qt::SolidLine, Qt::FlatCap);

    if(isHeader)
    {
        //Keep width independent of view scale but with limits
        qreal wt = pageLay.pageMarginsPenWidth / viewScaleFactor;
        wt = qBound(2.0, wt, 20.0);
        borderPen.setWidthF(wt);
    }

    painter->setPen(borderPen);

    QVector<QLineF> vec;

    if(!isHeader || orient == Qt::Horizontal)
    {
        //Horizontal header draws vertical borders
        int nLines = lastPageVertBorder - firstPageVertBorder + 1;
        vec.reserve(nLines);

        qreal borderX = fixedOffsetX + firstPageVertBorder * effectivePageSize.width();
        for(int i = 0; i < nLines; i++)
        {
            QLineF line(borderX, pageBordersTop, borderX, pageBordersBottom);
            vec.append(line);
            borderX += effectivePageSize.width();
        }
        painter->drawLines(vec);
    }

    vec.clear();

    if(!isHeader || orient == Qt::Vertical)
    {
        //Vertical header draws horizontal borders
        int nLines = lastPageHorizBorder - firstPageHorizBorder + 1;
        vec.reserve(nLines);

        qreal borderY = fixedOffsetY + firstPageHorizBorder * effectivePageSize.height();
        for(int i = 0; i < nLines; i++)
        {
            QLineF line(pageBordersLeft, borderY, pageBordersRight, borderY);
            vec.append(line);
            borderY += effectivePageSize.height();
        }
        painter->drawLines(vec);
    }

}
