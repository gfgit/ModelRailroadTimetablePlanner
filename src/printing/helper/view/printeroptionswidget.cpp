#include "printeroptionswidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <QVBoxLayout>
#include <QGridLayout>

#include "utils/files/file_format_names.h"

#include "utils/owningqpointer.h"
#include <QFileDialog>
#include <QPrintDialog>
#include "custompagesetupdlg.h"

#include "sceneprintpreviewdlg.h"

#include <QPrinter>

#include <QFileInfo>
#include <QStandardPaths>

#include "utils/files/recentdirstore.h"

static const QLatin1String recentDirKey = QLatin1String("print_dir");

PrinterOptionsWidget::PrinterOptionsWidget(QWidget *parent) :
    QWidget(parent),
    m_printer(nullptr),
    m_sourceScene(nullptr)
{
    createFilesBox();
    createPageLayoutBox();

    outputTypeCombo = new QComboBox;
    QStringList items;
    items.reserve(int(Print::OutputType::NTypes));
    for(int i = 0; i < int(Print::OutputType::NTypes); i++)
        items.append(Print::getOutputTypeName(Print::OutputType(i)));
    outputTypeCombo->addItems(items);
    outputTypeCombo->setCurrentIndex(int(Print::OutputType::Pdf));
    connect(outputTypeCombo, qOverload<int>(&QComboBox::activated),
            this, &PrinterOptionsWidget::updateOutputType);

    QLabel *typeLabel = new QLabel(tr("Output Type:"));

    QGridLayout *lay = new QGridLayout(this);
    lay->addWidget(typeLabel, 0, 0);
    lay->addWidget(outputTypeCombo, 0, 1);
    lay->addWidget(fileBox, 1, 0, 1, 2);
    lay->addWidget(pageLayoutBox, 2, 0, 1, 2);
}

void PrinterOptionsWidget::setOptions(const Print::PrintBasicOptions &printOpt)
{
    pathEdit->setText(printOpt.filePath);
    patternEdit->setText(printOpt.fileNamePattern);
    differentFilesCheckBox->setChecked(printOpt.useOneFileForEachScene);
    sceneInOnePageCheckBox->setChecked(printOpt.printSceneInOnePage);
    outputTypeCombo->setCurrentIndex(int(printOpt.outputType));
    updateOutputType();

    if(printOpt.filePath.isEmpty())
    {
        pathEdit->setText(RecentDirStore::getDir(recentDirKey, RecentDirStore::Documents));
    }
}

Print::PrintBasicOptions PrinterOptionsWidget::getOptions() const
{
    Print::PrintBasicOptions printOpt;
    printOpt.filePath = pathEdit->text();
    printOpt.fileNamePattern = patternEdit->text();
    printOpt.useOneFileForEachScene = differentFilesCheckBox->isChecked();
    printOpt.printSceneInOnePage = sceneInOnePageCheckBox->isChecked();
    printOpt.outputType = Print::OutputType(outputTypeCombo->currentIndex());
    return printOpt;
}

bool PrinterOptionsWidget::validateOptions()
{
    if(outputTypeCombo->currentIndex() != int(Print::OutputType::Native))
    {
        //Check files
        const QString path = pathEdit->text();
        if(path.isEmpty())
        {
            return false;
        }

        const QString pattern = patternEdit->text();
        if(pattern.isEmpty() && differentFilesCheckBox->isChecked())
        {
            return false;
        }
    }

    if(!pathEdit->text().isEmpty())
    {
        RecentDirStore::setPath(recentDirKey, pathEdit->text());
    }

    return true;
}

bool PrinterOptionsWidget::isComplete() const
{
    if(outputTypeCombo->currentIndex() == int(Print::OutputType::Native))
        return true; //No need to check files

    bool complete = !pathEdit->text().isEmpty();
    if(complete && differentFilesCheckBox->isChecked())
        complete = !patternEdit->text().isEmpty();
    return complete;
}

QPrinter *PrinterOptionsWidget::printer() const
{
    return m_printer;
}

void PrinterOptionsWidget::setPrinter(QPrinter *newPrinter)
{
    m_printer = newPrinter;
}

Print::PageLayoutOpt PrinterOptionsWidget::getScenePageLay() const
{
    return scenePageLay;
}

void PrinterOptionsWidget::setScenePageLay(const Print::PageLayoutOpt &newScenePageLay)
{
    scenePageLay = newScenePageLay;
}

IGraphScene *PrinterOptionsWidget::sourceScene() const
{
    return m_sourceScene;
}

void PrinterOptionsWidget::setSourceScene(IGraphScene *newSourceScene)
{
    m_sourceScene = newSourceScene;
}

void PrinterOptionsWidget::onChooseFile()
{
    QString path = pathEdit->text();

    if(!path.isEmpty())
    {
        //Check if file or dir is valid
        QFileInfo info(path);
        if(differentFilesCheckBox->isChecked())
        {
            if(!info.isDir() || !info.exists())
                path.clear();
        }
        else
        {
            if(info.isFile() && !info.absoluteDir().exists())
                path.clear();
        }
    }

    //Default to Documents folder
    if(path.isEmpty())
        path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    if(differentFilesCheckBox->isChecked())
    {
        path = QFileDialog::getExistingDirectory(this,
                                                 tr("Choose Folder"),
                                                 path);

        if(path.isEmpty()) //User canceled dialog
            return;
    }
    else
    {
        QString ext;
        QString fullName;
        switch (Print::OutputType(outputTypeCombo->currentIndex()))
        {
        case Print::OutputType::Pdf:
            ext = QStringLiteral(".pdf");
            fullName = FileFormats::tr(FileFormats::pdfFile);
            break;
        case Print::OutputType::Svg:
            ext = QStringLiteral(".svg");
            fullName = FileFormats::tr(FileFormats::svgFile);
            break;
        default:
            break;
        }

        path = QFileDialog::getSaveFileName(this,
                                            tr("Choose file"),
                                            path,
                                            fullName);
        if(path.isEmpty()) //User canceled dialog
            return;

        if(!path.endsWith(ext))
            path.append(ext);
    }

    pathEdit->setText(QDir::fromNativeSeparators(path));
}

void PrinterOptionsWidget::updateDifferentFiles()
{
    //Pattern is applicable only if printing multiple files
    patternEdit->setEnabled(differentFilesCheckBox->isChecked());

    //If pathEdit contains a file but user checks 'Different Files'
    //We go up to file directory and use that

    QString path = pathEdit->text();
    if(path.isEmpty())
        return;

    QString ext;
    switch (Print::OutputType(outputTypeCombo->currentIndex()))
    {
    case Print::OutputType::Pdf:
        ext = QStringLiteral(".pdf");
        break;
    case Print::OutputType::Svg:
        ext = QStringLiteral(".svg");
        break;
    default:
        break;
    }

    if(differentFilesCheckBox->isChecked())
    {
        if(path.endsWith(ext))
        {
            path = path.left(path.lastIndexOf('/'));
        }
    }
    else
    {
        if(!path.endsWith(ext))
        {
            if(path.endsWith('/'))
                path.append(QStringLiteral("file%1").arg(ext));
            else
                path.append(QStringLiteral("/file%1").arg(ext));
        }
    }

    pathEdit->setText(path);
}

void PrinterOptionsWidget::onOpenPageSetup()
{
    if(!m_printer)
        return;

    QPageLayout pageLay;

    if(m_printer->outputFormat() == QPrinter::NativeFormat)
    {
        //Native dialog for native printer
        OwningQPointer<QPrintDialog> dlg = new QPrintDialog(m_printer, this);
        dlg->exec();

        //Get after dialog finished
        pageLay = m_printer->pageLayout();

        //Fix possible wrong page size
        QPageSize pageSz = pageLay.pageSize();
        QPageLayout::Orientation orient = pageLay.orientation();
        pageSz = PrintHelper::fixPageSize(pageSz, orient);
        pageLay.setPageSize(pageSz);
        pageLay.setOrientation(orient);
    }
    else
    {
        //Custom dialog for PDF printer
        OwningQPointer<CustomPageSetupDlg> dlg = new CustomPageSetupDlg(this);

        pageLay = m_printer->pageLayout();
        dlg->setPageSize(pageLay.pageSize());
        dlg->setPageOrient(pageLay.orientation());
        if(dlg->exec() != QDialog::Accepted || !dlg)
            return;

        //Update layout page size
        pageLay.setPageSize(dlg->getPageSize());
        pageLay.setOrientation(dlg->getPageOrient());
    }

    //Update printer layout
    m_printer->setPageLayout(pageLay);

    //Apply page size to scene layout
    PrintHelper::applyPageSize(pageLay.pageSize(), pageLay.orientation(), scenePageLay);
}

void PrinterOptionsWidget::onShowPreviewDlg()
{
    OwningQPointer<ScenePrintPreviewDlg> dlg = new ScenePrintPreviewDlg(this);

    dlg->setSourceScene(m_sourceScene);
    dlg->setScenePageLay(scenePageLay);
    dlg->setPrinter(m_printer);

    dlg->exec();

    if(!dlg)
        return;

    if(m_printer && m_printer->outputFormat() == QPrinter::PdfFormat)
    {
        //Update page layout
        m_printer->setPageLayout(dlg->getPrinterPageLay());
    }

    scenePageLay = dlg->getScenePageLay();
}

void PrinterOptionsWidget::updateOutputType()
{
    Print::OutputType type = Print::OutputType(outputTypeCombo->currentIndex());

    fileBox->setEnabled(type != Print::OutputType::Native);

    pageLayoutBox->setEnabled(type != Print::OutputType::Svg);
    pageSetupDlgBut->setText(type == Print::OutputType::Native ? tr("Printer Options") : tr("Page Setup"));

    if(type == Print::OutputType::Svg)
    {
        //Svg can only be printed in multiple files
        differentFilesCheckBox->setChecked(true);
        differentFilesCheckBox->setEnabled(false);

        //Svg will always be on single page
        sceneInOnePageCheckBox->setChecked(true);
        sceneInOnePageCheckBox->setEnabled(false);
    }
    else
    {
        differentFilesCheckBox->setEnabled(true);

        //Printers always need to split in multiple pages
        if(type == Print::OutputType::Native)
        {
            sceneInOnePageCheckBox->setChecked(false);
            sceneInOnePageCheckBox->setEnabled(false);
        }
        else
        {
            sceneInOnePageCheckBox->setEnabled(true);
        }
    }

    if(m_printer)
    {
        m_printer->setOutputFormat(type == Print::OutputType::Native ? QPrinter::NativeFormat : QPrinter::PdfFormat);
    }

    //Check if new otptions are valid
    emit completeChanged();
}

void PrinterOptionsWidget::createFilesBox()
{
    fileBox = new QGroupBox(tr("Files"));

    differentFilesCheckBox = new QCheckBox(tr("Different Files"));
    connect(differentFilesCheckBox, &QCheckBox::toggled,
            this, &PrinterOptionsWidget::updateDifferentFiles);

    sceneInOnePageCheckBox = new QCheckBox(tr("Whole scene in one page"));
    sceneInOnePageCheckBox->setToolTip(tr("This will print a custom page size to fit everything in one page.\n"
                                          "Not available on native printer."));

    pathEdit = new QLineEdit;
    connect(pathEdit, &QLineEdit::textChanged, this, &PrinterOptionsWidget::completeChanged);

    patternEdit = new QLineEdit;
    connect(patternEdit, &QLineEdit::textChanged, this, &PrinterOptionsWidget::completeChanged);
    patternEdit->setEnabled(false); //Initially different files is not checked

    fileBut = new QPushButton(tr("Choose"));
    connect(fileBut, &QPushButton::clicked, this, &PrinterOptionsWidget::onChooseFile);

    QLabel *label = new QLabel(tr("File(s)"));

    QGridLayout *l = new QGridLayout;
    l->addWidget(differentFilesCheckBox, 0, 0, 1, 2);
    l->addWidget(sceneInOnePageCheckBox, 1, 0, 1, 2);
    l->addWidget(label, 2, 0, 1, 2);
    l->addWidget(pathEdit, 3, 0);
    l->addWidget(fileBut, 3, 1);
    l->addWidget(patternEdit, 4, 0);
    fileBox->setLayout(l);

    QString patternHelp = tr("File name pattern:<br>"
                             "<b>%n</b> Name_with_underscores<br>"
                             "<b>%N</b> Name with spaces<br>"
                             "<b>%t</b> Type (Station, ...)<br>"
                             "<b>%i</b> Progressive number");
    patternEdit->setToolTip(patternHelp);
}

void PrinterOptionsWidget::createPageLayoutBox()
{
    pageLayoutBox = new QGroupBox(tr("Page Layout"));

    pageSetupDlgBut = new QPushButton; //Text is set in updateOutputType()
    connect(pageSetupDlgBut, &QPushButton::clicked, this, &PrinterOptionsWidget::onOpenPageSetup);

    previewDlgBut = new QPushButton(tr("Preview"));
    connect(previewDlgBut, &QPushButton::clicked, this, &PrinterOptionsWidget::onShowPreviewDlg);

    QVBoxLayout *l = new QVBoxLayout(pageLayoutBox);
    l->addWidget(pageSetupDlgBut);
    l->addWidget(previewDlgBut);
}
