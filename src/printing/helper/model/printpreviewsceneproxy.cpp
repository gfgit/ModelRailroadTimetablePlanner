#include "printpreviewsceneproxy.h"

#include <QPainter>

PrintPreviewSceneProxy::PrintPreviewSceneProxy(QObject *parent) :
    IGraphScene(parent),
    sourceScene(nullptr),
    sourceScaleFactor(1)
{
    m_cachedHeaderSize = QSizeF(50, 50);
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
        sourceRect = QRectF(sourceRect.topLeft() / sourceScaleFactor,
                            sourceRect.size() / sourceScaleFactor);

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
        painter->scale(sourceScaleFactor, sourceScaleFactor);

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
}

void PrintPreviewSceneProxy::renderHeader(QPainter *painter, const QRectF &sceneRect, Qt::Orientation orient)
{
    painter->fillRect(sceneRect, QColor(255, 0, 0, 40));

    //TODO: draw page number header
    if(sourceScene)
        sourceScene->renderHeader(painter, sceneRect, orient);
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

void PrintPreviewSceneProxy::onSourceSceneDestroyed()
{
    sourceScene = nullptr;
    updateSourceSizeAndRedraw();
}

void PrintPreviewSceneProxy::updateSourceSizeAndRedraw()
{
    if(sourceScene)
    {
        //Scale source scene and then add our headers
        m_cachedContentsSize = sourceScene->getContentsSize() * sourceScaleFactor + m_cachedHeaderSize;
    }
    else
    {
        m_cachedContentsSize = QSizeF();
    }

    emit headersSizeChanged();
    emit redrawGraph();
}

double PrintPreviewSceneProxy::getSourceScaleFactor() const
{
    return sourceScaleFactor;
}

void PrintPreviewSceneProxy::setSourceScaleFactor(double newSourceScaleFactor)
{
    sourceScaleFactor = newSourceScaleFactor;
    updateSourceSizeAndRedraw();
}
