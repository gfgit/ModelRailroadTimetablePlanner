#ifndef SCENEPRINTPREVIEWDLG_H
#define SCENEPRINTPREVIEWDLG_H

#include <QDialog>

#include <QPageSize>

class BasicGraphView;

class IGraphScene;
class PrintPreviewSceneProxy;

class QSlider;
class QSpinBox;
class QDoubleSpinBox;

class QPrinter;

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

    QPrinter *printer() const;
    void setPrinter(QPrinter *newPrinter);

    inline QPageSize getPageSize() const { return m_pageSize; }
    inline Qt::Orientation getPageOrient() const { return m_pageOrient; }
    void setPageSize(const QPageSize &newPageSize, Qt::Orientation orient);

private slots:
    void updateZoomLevel(int zoom);
    void onScaleChanged(double zoom);

    void showPageSetupDlg();

private:
    void updateModelPageSize();

private:
    BasicGraphView *graphView;
    PrintPreviewSceneProxy *previewScene;

    QSlider *zoomSlider;
    QSpinBox *zoomSpinBox;

    QDoubleSpinBox *sceneScaleSpinBox;
    double mSceneScale;

    QPrinter *m_printer;
    QPageSize m_pageSize;
    Qt::Orientation m_pageOrient;
};

#endif // SCENEPRINTPREVIEWDLG_H
