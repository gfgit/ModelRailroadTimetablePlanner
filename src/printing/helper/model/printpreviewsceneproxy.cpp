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
}

void PrintPreviewSceneProxy::renderContents(QPainter *painter, const QRectF &sceneRect)
{
    const QTransform originalTransform = painter->worldTransform();

    if(sourceScene)
    {
        //Map coordinates to source scene
        QRectF sourceRect = sceneRect;

        //Shift contents by our headers size
        QPointF origin(m_cachedHeaderSize.width(), m_cachedHeaderSize.height());
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
            sourceScene->renderHeader(painter, horizHeaderRect, Qt::Horizontal);
        }

        //If on left draw vertical header
        if(sourceRect.left() < headerSize.width())
        {
            QRectF vertHeaderRect = sourceRect;
            vertHeaderRect.setRight(headerSize.width());
            sourceScene->renderHeader(painter, vertHeaderRect, Qt::Vertical);
        }
    }

    //Reset transorm to previous
    painter->setWorldTransform(originalTransform);

    //TODO: draw page borders and margins on top of scene
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

    //Do not go under headers
    const qreal pageBordersLeft = qMax(m_cachedHeaderSize.width(), sceneRect.left());
    const qreal pageBordersTop = qMax(m_cachedHeaderSize.height(), sceneRect.top());

    //Do not exceed scene proxy size
    const qreal pageBordersRight = qMin(m_cachedContentsSize.width(), sceneRect.right());
    const qreal pageBordersBottom = qMin(m_cachedContentsSize.height(), sceneRect.bottom());

    //Draw effective page borders
    QPen borderPen(Qt::black, 5, Qt::SolidLine, Qt::FlatCap);
    painter->setPen(borderPen);

    QVector<QLineF> vec;

    //Vertical borders
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
    vec.clear();

    //Horizontal borders
    nLines = lastPageHorizBorder - firstPageHorizBorder + 1;
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

void PrintPreviewSceneProxy::renderHeader(QPainter *painter, const QRectF &sceneRect, Qt::Orientation orient)
{
    painter->fillRect(sceneRect, QColor(255, 0, 0, 40));

    //TODO: draw page number header
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
