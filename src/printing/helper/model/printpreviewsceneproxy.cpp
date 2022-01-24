#include "printpreviewsceneproxy.h"

#include "utils/font_utils.h"

#include <QPainter>

#include <QtMath>

PrintPreviewSceneProxy::PrintPreviewSceneProxy(QObject *parent) :
    IGraphScene(parent),
    m_sourceScene(nullptr),
    viewScaleFactor(1)
{
    originalHeaderSize = QSizeF(70, 70);
    setViewScaleFactor(1.0);
}

void PrintPreviewSceneProxy::renderContents(QPainter *painter, const QRectF &sceneRect)
{
    const QTransform originalTransform = painter->worldTransform();

    if(m_sourceScene)
    {
        //Map coordinates to source scene
        QRectF sourceRect = sceneRect;

        //Shift contents by our headers size and top left page margin
        QPointF origin(m_cachedHeaderSize.width() + m_pageLay.marginOriginalWidth,
                       m_cachedHeaderSize.height() + m_pageLay.marginOriginalWidth);
        sourceRect.moveTopLeft(sourceRect.topLeft() - origin);

        //Apply scale factor
        sourceRect = QRectF(sourceRect.topLeft() / m_pageLay.premultipliedScaleFactor,
                            sourceRect.size() / m_pageLay.premultipliedScaleFactor);

        //Cut negative part which would go under our headers
        //This will reduce a bit the rect size
        if(sourceRect.left() < 0)
            sourceRect.setLeft(0);
        if(sourceRect.top() < 0)
            sourceRect.setTop(0);

        //Cut possible extra parts
        QSizeF contentsSize = m_sourceScene->getContentsSize();
        if(sourceRect.right() > contentsSize.width())
            sourceRect.setRight(contentsSize.width());
        if(sourceRect.bottom() > contentsSize.height())
            sourceRect.setBottom(contentsSize.height());

        painter->translate(origin);
        painter->scale(m_pageLay.premultipliedScaleFactor, m_pageLay.premultipliedScaleFactor);

        //Draw source contents
        m_sourceScene->renderContents(painter, sourceRect);

        QSizeF headerSize = m_sourceScene->getHeaderSize();

        //If on top draw horizontal header
        if(sourceRect.top() < headerSize.height())
        {
            QRectF horizHeaderRect = sourceRect;
            horizHeaderRect.setBottom(headerSize.height());
            m_sourceScene->renderHeader(painter, horizHeaderRect, Qt::Horizontal, 0);
        }

        //If on left draw vertical header
        if(sourceRect.left() < headerSize.width())
        {
            QRectF vertHeaderRect = sourceRect;
            vertHeaderRect.setRight(headerSize.width());
            m_sourceScene->renderHeader(painter, vertHeaderRect, Qt::Vertical, 0);
        }

        //Wash out a bit to make page borders more visible
        painter->fillRect(sourceRect, QColor(40, 255, 40, 100));
    }

    //Reset transorm to previous
    painter->setWorldTransform(originalTransform);

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

    drawPageBorders(painter, sceneRect, true, orient);
}

IGraphScene *PrintPreviewSceneProxy::getSourceScene() const
{
    return m_sourceScene;
}

void PrintPreviewSceneProxy::setSourceScene(IGraphScene *newSourceScene)
{
    //Connect to size and content changes to forward them to view
    //Since this is a print preview we are not interested
    //in sceneActivated() and requestShowRect() signals

    if(m_sourceScene)
    {
        disconnect(m_sourceScene, &QObject::destroyed, this,
                   &PrintPreviewSceneProxy::onSourceSceneDestroyed);
        disconnect(m_sourceScene, &IGraphScene::redrawGraph,
                   this, &PrintPreviewSceneProxy::updateSourceSizeAndRedraw);
        disconnect(m_sourceScene, &IGraphScene::headersSizeChanged,
                   this, &PrintPreviewSceneProxy::updateSourceSizeAndRedraw);
    }
    m_sourceScene = newSourceScene;
    if(m_sourceScene)
    {
        connect(m_sourceScene, &QObject::destroyed, this,
                &PrintPreviewSceneProxy::onSourceSceneDestroyed);
        connect(m_sourceScene, &IGraphScene::redrawGraph,
                this, &PrintPreviewSceneProxy::updateSourceSizeAndRedraw);
        connect(m_sourceScene, &IGraphScene::headersSizeChanged,
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

PrintHelper::PageLayoutOpt PrintPreviewSceneProxy::getPageLay() const
{
    return m_pageLay;
}

void PrintPreviewSceneProxy::setPageLay(const PrintHelper::PageLayoutOpt &newPageLay)
{
    m_pageLay = newPageLay;
    updateSourceSizeAndRedraw();
}

void PrintPreviewSceneProxy::onSourceSceneDestroyed()
{
    m_sourceScene = nullptr;
    updateSourceSizeAndRedraw();
}

void PrintPreviewSceneProxy::updateSourceSizeAndRedraw()
{
    PrintHelper::updatePrinterResolution(m_pageLay.printerResolution, m_pageLay);

    PrintHelper::calculatePageCount(m_sourceScene, m_pageLay, effectivePageSize);

    //Calculate total size
    QSizeF printedSize(m_pageLay.horizPageCnt * m_pageLay.devicePageRect.width(),
                       m_pageLay.vertPageCnt * m_pageLay.devicePageRect.height());

    //Adjust by substracting overlapped edges ((2 * n) - 2)
    printedSize -= QSizeF((2 * m_pageLay.horizPageCnt - 2) * m_pageLay.marginOriginalWidth,
                          (2 * m_pageLay.vertPageCnt - 2)* m_pageLay.marginOriginalWidth);

    //Add our headers and store
    m_cachedContentsSize = printedSize + m_cachedHeaderSize;

    emit redrawGraph();
    emit pageCountChanged();
}

void PrintPreviewSceneProxy::drawPageBorders(QPainter *painter, const QRectF &sceneRect, bool isHeader, Qt::Orientation orient)
{
    const qreal fixedOffsetX = m_cachedHeaderSize.width() + m_pageLay.marginOriginalWidth;
    const qreal fixedOffsetY = m_cachedHeaderSize.height() + m_pageLay.marginOriginalWidth;

    //Theese are effective page sizes (so inside margins)
    int firstPageVertBorder = qCeil((sceneRect.left() - fixedOffsetX) / effectivePageSize.width());
    int lastPageVertBorder = qFloor((sceneRect.right() - fixedOffsetX) / effectivePageSize.width());

    //For N pages there are N + 1 borders, but we count from 0 so last border is N

    //Vertical border, horizontal page count
    if(lastPageVertBorder > m_pageLay.horizPageCnt)
        lastPageVertBorder = m_pageLay.horizPageCnt;
    if(firstPageVertBorder > lastPageVertBorder)
        firstPageVertBorder = lastPageVertBorder;
    if(firstPageVertBorder < 0)
        firstPageVertBorder = 0;

    int firstPageHorizBorder = qCeil((sceneRect.top() - fixedOffsetY) / effectivePageSize.height());
    int lastPageHorizBorder = qFloor((sceneRect.bottom() - fixedOffsetY) / effectivePageSize.height());

    //Horizontal border, vertical page count
    if(lastPageHorizBorder > m_pageLay.vertPageCnt)
        lastPageHorizBorder = m_pageLay.vertPageCnt;
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

    QPen pageMarginsPen(Qt::red, m_pageLay.pageMarginsPenWidth, Qt::SolidLine, Qt::FlatCap);

    if(isHeader)
    {
        //Keep width independent of view scale but with limits
        qreal wt = m_pageLay.pageMarginsPenWidth / viewScaleFactor;
        wt = qBound(2.0, wt, 20.0);
        pageMarginsPen.setWidthF(wt);
    }

    QTextOption pageNumTextOpt(Qt::AlignCenter);

    QFont pageNumFont;
    pageNumFont.setBold(true);
    if(isHeader)
    {
        setFontPointSizeDPI(pageNumFont, m_cachedHeaderSize.height() * 0.6, painter);
    }
    else
    {
        const double minEdge = qMin(effectivePageSize.width(), effectivePageSize.height());
        setFontPointSizeDPI(pageNumFont, minEdge * 0.4, painter);
    }
    painter->setFont(pageNumFont);

    QVector<QLineF> marginsVec;

    int nLinesVert = lastPageVertBorder - firstPageVertBorder + 1;
    int nLinesHoriz = lastPageHorizBorder - firstPageHorizBorder + 1;

    if(!isHeader)
    {
        //Draw real page borders (outside margins)

        //Set pen for page numbers
        painter->setPen(pageMarginsPen);

        /* Divide them in 3 color groups
         * Each row shifts by one
         *
         * +---+---+---+---+
         * | 1 | 2 | 3 | 1 |
         * +---+---+---+---+
         * | 2 | 3 | 1 | 2 |
         * +---+---+---+---+
         * | 3 | 1 | 2 | 3 |
         * +---+---+---+---+
         */

        const double borderPenWidth = m_pageLay.pageMarginsPenWidth * 0.7;

        constexpr const int NPageColors = 3;
        QPen pagesBorderPen[NPageColors] = {
            QPen(Qt::blue,    borderPenWidth, Qt::DashLine, Qt::FlatCap),
            QPen(Qt::magenta, borderPenWidth, Qt::DashLine, Qt::FlatCap),
            QPen(Qt::black,   borderPenWidth, Qt::DashLine, Qt::FlatCap)
        };

        //FIXME: allocate memory in advance
        QVector<QLineF> pageBordersVec[NPageColors];

        int vertPageColorGroup = firstPageVertBorder % NPageColors;
        int horizPageColorGroup = 0;

        //Get page rect, move out of header
        QRectF pageRect = m_pageLay.devicePageRect;

        //Try also previous than first or next of last
        //This is because maybe real border is shown but not the effective margin
        for(int x = -1; x < nLinesVert + 1; x++)
        {
            //Increment vertical page color group
            vertPageColorGroup = (vertPageColorGroup + 1) % NPageColors;

            const int pageBorderCol = firstPageVertBorder + x;
            if(pageBorderCol < 0 || pageBorderCol >= m_pageLay.horizPageCnt)
                continue; //Before first or after last, skip

            pageRect.moveLeft(m_cachedHeaderSize.width() + effectivePageSize.width() * (pageBorderCol));

            //Reset column to original value for every row
            horizPageColorGroup = firstPageHorizBorder % NPageColors;

            for(int y = -1; y < nLinesHoriz + 1; y++)
            {
                //Increment horizontal page color group
                horizPageColorGroup = (horizPageColorGroup + 1) % NPageColors;

                const int pageBorderRow = firstPageHorizBorder + y;
                if(pageBorderRow < 0 || pageBorderRow >= m_pageLay.vertPageCnt)
                    continue; //Before first or after last, skip

                pageRect.moveTop(m_cachedHeaderSize.height() + effectivePageSize.height() * (pageBorderRow));

                //Draw page number
                const int pageNumber = pageBorderRow * m_pageLay.horizPageCnt + pageBorderCol + 1;
                painter->drawText(pageRect, QString::number(pageNumber), pageNumTextOpt);

                //Calculate page borders
                QLineF topBorder = QLineF(pageRect.topLeft(), pageRect.topRight());
                QLineF bottomBorder = QLineF(pageRect.bottomLeft(), pageRect.bottomRight());
                QLineF leftBorder = QLineF(pageRect.topLeft(), pageRect.bottomLeft());
                QLineF rightBorder = QLineF(pageRect.topRight(), pageRect.bottomRight());

                const int group = (vertPageColorGroup + horizPageColorGroup) % NPageColors;

                pageBordersVec[group].append(topBorder);
                pageBordersVec[group].append(bottomBorder);
                pageBordersVec[group].append(leftBorder);
                pageBordersVec[group].append(rightBorder);
            }
        }

        //Draw lines
        for(int i = 0; i < NPageColors; i++)
        {
            painter->setPen(pagesBorderPen[i]);
            painter->drawLines(pageBordersVec[i]);
        }
    }

    //Set pen for page margins
    painter->setPen(pageMarginsPen);

    //Draw effective page borders
    if(!isHeader || orient == Qt::Horizontal)
    {
        //Horizontal header draws vertical borders
        marginsVec.reserve(nLinesVert);

        qreal borderX = fixedOffsetX + firstPageVertBorder * effectivePageSize.width();
        for(int i = 0; i < nLinesVert; i++)
        {
            QLineF line(borderX, pageBordersTop, borderX, pageBordersBottom);
            marginsVec.append(line);
            borderX += effectivePageSize.width();
        }
        painter->drawLines(marginsVec);

        if(isHeader)
        {
            //Draw page numbers
            int firstPageNumber = firstPageVertBorder;
            if(firstPageVertBorder > 0)
                firstPageNumber--; //Draw also previous page

            //If there are N pages, there are (N + 1) margins
            //So last margin and last but one margin both belong to last page
            int lastPageNumberPlusOne = lastPageVertBorder;
            if(lastPageVertBorder < m_pageLay.horizPageCnt)
                lastPageNumberPlusOne++; //Draw also next page

            borderX = fixedOffsetX + firstPageNumber * effectivePageSize.width();
            QRectF textRect(borderX, 0, effectivePageSize.width(), m_cachedHeaderSize.height());
            for(int i = firstPageNumber; i < lastPageNumberPlusOne; i++)
            {
                //Column index starts from 0 so add +1
                painter->drawText(textRect, QString::number(i + 1), pageNumTextOpt);
                textRect.moveLeft(textRect.left() + effectivePageSize.width());
            }
        }
    }

    marginsVec.clear();

    if(!isHeader || orient == Qt::Vertical)
    {
        //Vertical header draws horizontal borders
        marginsVec.reserve(nLinesHoriz);

        qreal borderY = fixedOffsetY + firstPageHorizBorder * effectivePageSize.height();
        for(int i = 0; i < nLinesHoriz; i++)
        {
            QLineF line(pageBordersLeft, borderY, pageBordersRight, borderY);
            marginsVec.append(line);
            borderY += effectivePageSize.height();
        }
        painter->drawLines(marginsVec);

        if(isHeader)
        {
            //Draw page numbers
            int firstPageNumber = firstPageHorizBorder;
            if(firstPageHorizBorder > 0)
                firstPageNumber--; //Draw also previous page

            int lastPageNumberPlusOne = lastPageHorizBorder;
            if(lastPageHorizBorder < m_pageLay.vertPageCnt)
                lastPageNumberPlusOne++; //Draw also next page

            borderY = fixedOffsetY + firstPageNumber * effectivePageSize.height();
            QRectF textRect(0, borderY, m_cachedHeaderSize.width(), effectivePageSize.height());
            for(int i = firstPageNumber; i < lastPageNumberPlusOne; i++)
            {
                //Row index starts from 0 so add +1
                painter->drawText(textRect, QString::number(i + 1), pageNumTextOpt);
                textRect.moveTop(textRect.top() + effectivePageSize.height());
            }
        }
    }

    //Free memory
    marginsVec.clear();
    marginsVec.squeeze();
}
