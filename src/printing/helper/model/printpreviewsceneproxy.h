#ifndef PRINTPREVIEWSCENEPROXY_H
#define PRINTPREVIEWSCENEPROXY_H

#include "utils/scene/igraphscene.h"

#include "printhelper.h"

class PrintPreviewSceneProxy : public IGraphScene
{
    Q_OBJECT
public:
    PrintPreviewSceneProxy(QObject *parent = nullptr);

    virtual void renderContents(QPainter *painter, const QRectF& sceneRect) override;
    virtual void renderHeader(QPainter *painter, const QRectF& sceneRect, Qt::Orientation orient) override;

    IGraphScene *getSourceScene() const;
    void setSourceScene(IGraphScene *newSourceScene);

    double getSourceScaleFactor() const;
    void setSourceScaleFactor(double newSourceScaleFactor);

    QRectF getPageSize() const;
    void setPageSize(const QRectF &newPageSize);

private slots:
    void onSourceSceneDestroyed();
    void updatePageLay();

private:
    void updateSourceSizeAndRedraw();

private:
    IGraphScene *sourceScene;
    PrintHelper::PageLayoutOpt pageLay;
    QSizeF effectivePageSize;
};

#endif // PRINTPREVIEWSCENEPROXY_H
