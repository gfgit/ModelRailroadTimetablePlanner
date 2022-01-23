#ifndef SCENEPRINTPREVIEWDLG_H
#define SCENEPRINTPREVIEWDLG_H

#include <QDialog>

#include <QPageLayout>
#include "printing/helper/model/printhelper.h"

class BasicGraphView;

class IGraphScene;
class PrintPreviewSceneProxy;

class QSlider;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;

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

    inline QPageLayout getPrinterPageLay() const { return printerPageLay; }
    void setPrinterPageLay(const QPageLayout& pageLay);

    PrintHelper::PageLayoutOpt getScenePageLay() const;
    void setScenePageLay(const PrintHelper::PageLayoutOpt &newScenePageLay);

private slots:
    void updateZoomLevel(int zoom);
    void onScaleChanged(double zoom);
    void setMarginsWidth(double margins);

    void showPageSetupDlg();

    void updatePageCount();

private:
    void updateModelPageSize();

private:
    BasicGraphView *graphView;
    PrintPreviewSceneProxy *previewScene;

    QSlider *zoomSlider;
    QSpinBox *zoomSpinBox;

    QDoubleSpinBox *sceneScaleSpinBox;
    QDoubleSpinBox *marginSpinBox;

    QLabel *pageCountLabel;

    QPrinter *m_printer;
    QPageLayout printerPageLay;
};

#endif // SCENEPRINTPREVIEWDLG_H
