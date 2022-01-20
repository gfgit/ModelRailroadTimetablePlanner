#include "printpreviewsceneproxy.h"

#include <QPainter>

PrintPreviewSceneProxy::PrintPreviewSceneProxy(QObject *parent) :
    IGraphScene(parent),
    sourceScene(nullptr)
{
    m_cachedHeaderSize = QSizeF(50, 50);
}

void PrintPreviewSceneProxy::renderContents(QPainter *painter, const QRectF &sceneRect)
{
    //TODO: draw page borders and margins on top of scene
    if(sourceScene)
        sourceScene->renderContents(painter, sceneRect);
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
        m_cachedContentsSize = sourceScene->getContentsSize() + m_cachedHeaderSize;
    }
    else
    {
        m_cachedContentsSize = QSizeF();
    }

    emit headersSizeChanged();
    emit redrawGraph();
}
