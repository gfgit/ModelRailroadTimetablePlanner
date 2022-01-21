#include "sceneprintpreviewdlg.h"

#include "utils/scene/basicgraphview.h"
#include "printing/helper/model/printpreviewsceneproxy.h"

#include <QVBoxLayout>
#include <QToolBar>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>

#include <QEvent>

ScenePrintPreviewDlg::ScenePrintPreviewDlg(QWidget *parent) :
    QDialog(parent),
    mZoom(100),
    mSceneScale(1)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    QToolBar *toolBar = new QToolBar;
    lay->addWidget(toolBar);

    zoomSlider = new QSlider(Qt::Horizontal);
    zoomSlider->setRange(25, 400);
    zoomSlider->setTickPosition(QSlider::TicksBelow);
    zoomSlider->setTickInterval(50);
    zoomSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    zoomSlider->setValue(mZoom);
    zoomSlider->setToolTip(tr("Double click to reset zoom"));
    connect(zoomSlider, &QSlider::valueChanged, this,
            &ScenePrintPreviewDlg::updateZoomLevel);
    zoomSlider->installEventFilter(this);
    QAction *zoomAct = toolBar->addWidget(zoomSlider);
    zoomAct->setText(tr("Zoom:"));

    zoomSpinBox = new QSpinBox;
    zoomSpinBox->setRange(25, 400);
    zoomSpinBox->setValue(mZoom);
    zoomSpinBox->setSuffix(QChar('%'));
    zoomSpinBox->setToolTip(tr("Zoom"));
    connect(zoomSpinBox, qOverload<int>(&QSpinBox::valueChanged),
            this, &ScenePrintPreviewDlg::updateZoomLevel);
    toolBar->addWidget(zoomSpinBox);

    toolBar->addSeparator();

    sceneScaleSpinBox = new QDoubleSpinBox;
    sceneScaleSpinBox->setRange(25, 400);
    sceneScaleSpinBox->setValue(mSceneScale * 100);
    sceneScaleSpinBox->setSuffix(QChar('%'));
    sceneScaleSpinBox->setToolTip(tr("Source scale factor"));
    connect(sceneScaleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &ScenePrintPreviewDlg::onScaleChanged);
    QAction *scaleAct = toolBar->addWidget(sceneScaleSpinBox);
    scaleAct->setText(tr("Scale:"));

    graphView = new BasicGraphView;
    lay->addWidget(graphView);

    previewScene = new PrintPreviewSceneProxy(this);
    graphView->setScene(previewScene);

    resize(500, 600);
}

bool ScenePrintPreviewDlg::eventFilter(QObject *watched, QEvent *ev)
{
    if(watched == zoomSlider && ev->type() == QEvent::MouseButtonDblClick)
    {
        //Zoom Slider was double clicked, reset zoom level to 100
        updateZoomLevel(100);
    }

    return QDialog::eventFilter(watched, ev);
}

void ScenePrintPreviewDlg::setSourceScene(IGraphScene *sourceScene)
{
    previewScene->setSourceScene(sourceScene);
}

void ScenePrintPreviewDlg::setSceneScale(double scaleFactor)
{
    if(qFuzzyCompare(mSceneScale, scaleFactor))
        return;

    mSceneScale = scaleFactor;
    sceneScaleSpinBox->setValue(mSceneScale * 100);

    previewScene->setSourceScaleFactor(mSceneScale);
}

void ScenePrintPreviewDlg::updateZoomLevel(int zoom)
{
    if(mZoom == zoom)
        return;

    mZoom = zoom;
    zoomSlider->setValue(zoom);
    zoomSpinBox->setValue(zoom);
    graphView->setZoomLevel(zoom);
}

void ScenePrintPreviewDlg::onScaleChanged(double zoom)
{
    //Convert from 0%-100% to 0-1
    setSceneScale(zoom / 100);
}
