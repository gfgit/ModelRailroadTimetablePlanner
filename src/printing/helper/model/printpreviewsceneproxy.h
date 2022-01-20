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

private slots:
    void onSourceSceneDestroyed();
    void updateSourceSizeAndRedraw();

private:
    IGraphScene *sourceScene;
    double sourceScaleFactor;
};

#endif // PRINTPREVIEWSCENEPROXY_H
