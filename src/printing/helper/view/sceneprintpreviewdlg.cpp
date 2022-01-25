#include "sceneprintpreviewdlg.h"

#include "utils/scene/basicgraphview.h"
#include "printing/helper/model/printpreviewsceneproxy.h"

#include <QVBoxLayout>
#include <QToolBar>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>

#include "utils/owningqpointer.h"
#include "printing/helper/view/custompagesetupdlg.h"
#include <QPageSetupDialog>
#include <QPrinter>

#include <QEvent>

ScenePrintPreviewDlg::ScenePrintPreviewDlg(QWidget *parent) :
    QDialog(parent),
    m_printer(nullptr)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    QToolBar *toolBar = new QToolBar;
    lay->addWidget(toolBar);

    zoomSlider = new QSlider(Qt::Horizontal);
    zoomSlider->setRange(25, 400);
    zoomSlider->setTickPosition(QSlider::TicksBelow);
    zoomSlider->setTickInterval(50);
    zoomSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    zoomSlider->setValue(100);
    zoomSlider->setToolTip(tr("Double click to reset zoom"));
    connect(zoomSlider, &QSlider::valueChanged, this,
            &ScenePrintPreviewDlg::updateZoomLevel);
    zoomSlider->installEventFilter(this);
    QAction *zoomAct = toolBar->addWidget(zoomSlider);
    zoomAct->setText(tr("Zoom:"));

    zoomSpinBox = new QSpinBox;
    zoomSpinBox->setRange(25, 400);
    zoomSpinBox->setValue(100);
    zoomSpinBox->setSuffix(QChar('%'));
    zoomSpinBox->setToolTip(tr("Zoom"));
    connect(zoomSpinBox, qOverload<int>(&QSpinBox::valueChanged),
            this, &ScenePrintPreviewDlg::updateZoomLevel);
    toolBar->addWidget(zoomSpinBox);

    toolBar->addSeparator();

    sceneScaleSpinBox = new QDoubleSpinBox;
    sceneScaleSpinBox->setRange(5, 200);
    sceneScaleSpinBox->setValue(100);
    sceneScaleSpinBox->setSuffix(QChar('%'));
    sceneScaleSpinBox->setToolTip(tr("Source scale factor"));
    connect(sceneScaleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &ScenePrintPreviewDlg::onScaleChanged);
    QAction *scaleAct = toolBar->addWidget(sceneScaleSpinBox);
    scaleAct->setText(tr("Scale:"));

    marginSpinBox = new QDoubleSpinBox;
    marginSpinBox->setRange(0, 200);
    marginSpinBox->setValue(20);
    marginSpinBox->setSuffix(QLatin1String(" pt")); //Points
    marginSpinBox->setToolTip(tr("Margins"));
    connect(marginSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &ScenePrintPreviewDlg::setMarginsWidth);
    QAction *marginsAct = toolBar->addWidget(marginSpinBox);
    marginsAct->setText(tr("Margins:"));

    toolBar->addSeparator();
    toolBar->addAction(tr("Setup Page"), this, &ScenePrintPreviewDlg::showPageSetupDlg);

    toolBar->addSeparator();
    pageCountLabel = new QLabel;
    toolBar->addWidget(pageCountLabel);

    graphView = new BasicGraphView;
    lay->addWidget(graphView);

    previewScene = new PrintPreviewSceneProxy(this);
    graphView->setScene(previewScene);
    connect(previewScene, &PrintPreviewSceneProxy::pageCountChanged,
            this, &ScenePrintPreviewDlg::updatePageCount);

    //Default to A4 Portraint page
    printerPageLay.setPageSize(QPageSize(QPageSize::A4));
    printerPageLay.setOrientation(QPageLayout::Portrait);
    updateModelPageSize();

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
    PrintHelper::PageLayoutOpt pageLay = previewScene->getPageLay();
    if(qFuzzyCompare(pageLay.sourceScaleFactor, scaleFactor))
        return;

    sceneScaleSpinBox->setValue(scaleFactor * 100);
    pageLay.sourceScaleFactor = scaleFactor;
    previewScene->setPageLay(pageLay);
}

QPrinter *ScenePrintPreviewDlg::printer() const
{
    return m_printer;
}

void ScenePrintPreviewDlg::setPrinter(QPrinter *newPrinter)
{
    m_printer = newPrinter;

    if(m_printer)
    {
        //Update page rect
        QPageLayout printerLay = m_printer->pageLayout();
        printerLay.setMargins(QMarginsF());
        setPrinterPageLay(printerLay);
    }
}

void ScenePrintPreviewDlg::setPrinterPageLay(const QPageLayout &pageLay)
{
    printerPageLay = pageLay;
    if(m_printer)
        m_printer->setPageLayout(printerPageLay);
    updateModelPageSize();
}

PrintHelper::PageLayoutOpt ScenePrintPreviewDlg::getScenePageLay() const
{
    return previewScene->getPageLay();
}

void ScenePrintPreviewDlg::setScenePageLay(const PrintHelper::PageLayoutOpt &newScenePageLay)
{
    previewScene->setPageLay(newScenePageLay);

    sceneScaleSpinBox->setValue(newScenePageLay.sourceScaleFactor * 100);
    marginSpinBox->setValue(newScenePageLay.marginWidthPoints);
}

void ScenePrintPreviewDlg::updateZoomLevel(int zoom)
{
    if(graphView->getZoomLevel() == zoom)
        return;

    //Set for view first because it checks minimum and mazimum values
    graphView->setZoomLevel(zoom);

    zoom = graphView->getZoomLevel();
    zoomSlider->setValue(zoom);
    zoomSpinBox->setValue(zoom);
    previewScene->setViewScaleFactor(double(zoom) / 100.0);
}

void ScenePrintPreviewDlg::onScaleChanged(double zoom)
{
    //Convert from 0%-100% to 0-1
    setSceneScale(zoom / 100);
}

void ScenePrintPreviewDlg::setMarginsWidth(double margins)
{
    PrintHelper::PageLayoutOpt pageLay = previewScene->getPageLay();
    if(qFuzzyCompare(pageLay.marginWidthPoints, margins))
        return;

    marginSpinBox->setValue(margins);
    pageLay.marginWidthPoints = margins;
    previewScene->setPageLay(pageLay);
}

void ScenePrintPreviewDlg::showPageSetupDlg()
{
    if(m_printer && m_printer->outputFormat() == QPrinter::NativeFormat)
    {
        //For native printers use standard page dialog
        OwningQPointer<QPageSetupDialog> dlg = new QPageSetupDialog(m_printer, this);
        dlg->exec();

        //Fix possible wrong page size
        QPageLayout pageLay = m_printer->pageLayout();
        QPageSize pageSz = pageLay.pageSize();
        QPageLayout::Orientation orient = pageLay.orientation();
        pageSz = PrintHelper::fixPageSize(pageSz, orient);
        pageLay.setPageSize(pageSz);
        pageLay.setOrientation(orient);
        pageLay.setMargins(QMarginsF()); //Reset margins
        m_printer->setPageLayout(pageLay);

        //Update page rect
        setPrinterPageLay(m_printer->pageLayout());
    }
    else
    {
        //Show custom dialog
        OwningQPointer<CustomPageSetupDlg> dlg = new CustomPageSetupDlg(this);
        dlg->setPageSize(printerPageLay.pageSize());
        dlg->setPageOrient(printerPageLay.orientation());
        if(dlg->exec() != QDialog::Accepted || !dlg)
            return;

        printerPageLay.setPageSize(dlg->getPageSize());
        printerPageLay.setOrientation(dlg->getPageOrient());

        //Update page rect
        setPrinterPageLay(printerPageLay);
    }
}

void ScenePrintPreviewDlg::updatePageCount()
{
    PrintHelper::PageLayoutOpt pageLay = previewScene->getPageLay();
    const int totalCount = pageLay.pageCountVert * pageLay.pageCountHoriz;
    pageCountLabel->setText(tr("Rows: %1, Cols: %2, Total Pages: %3")
                                .arg(pageLay.pageCountVert).arg(pageLay.pageCountHoriz).arg(totalCount));
}

void ScenePrintPreviewDlg::updateModelPageSize()
{
    PrintHelper::PageLayoutOpt pageLay = previewScene->getPageLay();

    QPageSize pageSize = printerPageLay.pageSize();
    QPageLayout::Orientation pageOrient = printerPageLay.orientation();
    PrintHelper::applyPageSize(pageSize, pageOrient, pageLay);

    previewScene->setPageLay(pageLay);
}
