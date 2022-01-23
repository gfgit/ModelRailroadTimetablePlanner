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
    virtual void renderHeader(QPainter *painter, const QRectF& sceneRect,
                              Qt::Orientation orient, double scroll) override;

    IGraphScene *getSourceScene() const;
    void setSourceScene(IGraphScene *newSourceScene);

    double getViewScaleFactor() const;
    void setViewScaleFactor(double newViewScaleFactor);

    double getSourceScaleFactor() const;
    void setSourceScaleFactor(double newSourceScaleFactor);

    QRectF getPageSize() const;
    void setPageSize(const QRectF &newPageSize);

    double getMarginWidth() const;
    void setMarginWidth(double newMarginWidth);

    PrintHelper::PageLayoutOpt getPageLay() const;
    void setPageLay(const PrintHelper::PageLayoutOpt& newPageLay);

private slots:
    void onSourceSceneDestroyed();
    void updateSourceSizeAndRedraw();

private:
    void updatePageLay();
    void drawPageBorders(QPainter *painter, const QRectF& sceneRect,
                         bool isHeader, Qt::Orientation orient = Qt::Horizontal);

private:
    IGraphScene *sourceScene;
    PrintHelper::PageLayoutOpt pageLay;
    QSizeF effectivePageSize;

    double viewScaleFactor;
    QSizeF originalHeaderSize;
};

#endif // PRINTPREVIEWSCENEPROXY_H
