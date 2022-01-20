#ifndef SCENEPRINTPREVIEWDLG_H
#define SCENEPRINTPREVIEWDLG_H

#include <QDialog>

class BasicGraphView;

class IGraphScene;
class PrintPreviewSceneProxy;

class QSlider;
class QSpinBox;

class ScenePrintPreviewDlg : public QDialog
{
    Q_OBJECT
public:
    explicit ScenePrintPreviewDlg(QWidget *parent = nullptr);

    void setSourceScene(IGraphScene *sourceScene);

    /*!
     * \brief listen to slider double click
     *
     * When \ref zoomSlider gets double clicked, reset zoom to 100%
     */
    bool eventFilter(QObject *watched, QEvent *ev) override;

private slots:
    void updateZoomLevel(int zoom);

private:
    BasicGraphView *graphView;
    PrintPreviewSceneProxy *previewScene;

    QSlider *zoomSlider;
    QSpinBox *zoomSpinBox;
    int mZoom;
};

#endif // SCENEPRINTPREVIEWDLG_H
