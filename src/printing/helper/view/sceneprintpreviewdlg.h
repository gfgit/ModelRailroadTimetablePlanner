#ifndef SCENEPRINTPREVIEWDLG_H
#define SCENEPRINTPREVIEWDLG_H

#include <QDialog>

class BasicGraphView;

class IGraphScene;
class PrintPreviewSceneProxy;

class QSlider;
class QSpinBox;
class QDoubleSpinBox;

class ScenePrintPreviewDlg : public QDialog
{
    Q_OBJECT
public:
    explicit ScenePrintPreviewDlg(QWidget *parent = nullptr);

    /*!
     * \brief listen to slider double click
     *
     * When \ref zoomSlider gets double clicked, reset zoom to 100%
     */
    bool eventFilter(QObject *watched, QEvent *ev) override;

    void setSourceScene(IGraphScene *sourceScene);

    void setSceneScale(double scaleFactor);

private slots:
    void updateZoomLevel(int zoom);
    void onScaleChanged(double zoom);

private:
    BasicGraphView *graphView;
    PrintPreviewSceneProxy *previewScene;

    QSlider *zoomSlider;
    QSpinBox *zoomSpinBox;

    QDoubleSpinBox *sceneScaleSpinBox;
    double mSceneScale;
};

#endif // SCENEPRINTPREVIEWDLG_H
