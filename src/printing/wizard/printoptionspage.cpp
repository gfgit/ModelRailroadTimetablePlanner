#include "printoptionspage.h"

#include "printwizard.h"

#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>

#include <QVBoxLayout>
#include <QGridLayout>

#include "utils/files/file_format_names.h"

#include "utils/owningqpointer.h"
#include <QFileDialog>
#include <QPrintDialog>
#include "printing/helper/view/custompagesetupdlg.h"

#include "printing/helper/view/sceneprintpreviewdlg.h"
#include "sceneselectionmodel.h"
#include "graph/model/linegraphscene.h"

//FIXME: remove
#include <QGuiApplication>
#include <QPrintPreviewDialog>
#include "printing/printworker.h"
#include <QPrinter>

#include <QFileInfo>
#include <QStandardPaths>


PrintOptionsPage::PrintOptionsPage(PrintWizard *w, QWidget *parent) :
    QWizardPage (parent),
    mWizard(w)
{
    createFilesBox();
    createPageLayoutBox();

    outputTypeCombo = new QComboBox;
    QStringList items;
    items.reserve(int(Print::NTypes));
    for(int i = 0; i < int(Print::NTypes); i++)
        items.append(Print::getOutputTypeName(Print::OutputType(i)));
    outputTypeCombo->addItems(items);
    connect(outputTypeCombo, qOverload<int>(&QComboBox::activated),
            this, &PrintOptionsPage::updateOutputType);

    QLabel *typeLabel = new QLabel(tr("Output Type:"));

    QGridLayout *lay = new QGridLayout(this);
    lay->addWidget(typeLabel, 0, 0);
    lay->addWidget(outputTypeCombo, 0, 1);
    lay->addWidget(fileBox, 1, 0, 1, 2);
    lay->addWidget(pageLayoutBox, 2, 0, 1, 2);

    setTitle(tr("Print Options"));

    setCommitPage(true);

    //Change 'Commit' to 'Print' so user understands better
    setButtonText(QWizard::CommitButton, tr("Print"));
}

PrintOptionsPage::~PrintOptionsPage()
{

}

void PrintOptionsPage::createFilesBox()
{
    fileBox = new QGroupBox(tr("Files"));

    differentFilesCheckBox = new QCheckBox(tr("Different Files"));
    connect(differentFilesCheckBox, &QCheckBox::toggled,
            this, &PrintOptionsPage::updateDifferentFiles);

    pathEdit = new QLineEdit;
    connect(pathEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);

    patternEdit = new QLineEdit;
    connect(patternEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    patternEdit->setEnabled(false); //Initially different files is not checked

    fileBut = new QPushButton(tr("Choose"));
    connect(fileBut, &QPushButton::clicked, this, &PrintOptionsPage::onChooseFile);

    QLabel *label = new QLabel(tr("File(s)"));

    QGridLayout *l = new QGridLayout;
    l->addWidget(differentFilesCheckBox, 0, 0, 1, 2);
    l->addWidget(label, 1, 0, 1, 2);
    l->addWidget(pathEdit, 2, 0);
    l->addWidget(fileBut, 2, 1);
    l->addWidget(patternEdit, 3, 0);
    fileBox->setLayout(l);

    QString patternHelp = tr("File name pattern:<br>"
                             "<b>%n</b> Name_with_underscores<br>"
                             "<b>%N</b> Name with spaces<br>"
                             "<b>%t</b> Type (Station, ...)<br>"
                             "<b>%i</b> Progressive number");
    patternEdit->setToolTip(patternHelp);
}

void PrintOptionsPage::createPageLayoutBox()
{
    pageLayoutBox = new QGroupBox(tr("Page Layout"));

    pageSetupDlgBut = new QPushButton; //Text is set in updateOutputType()
    connect(pageSetupDlgBut, &QPushButton::clicked, this, &PrintOptionsPage::onOpenPageSetup);

    previewDlgBut = new QPushButton(tr("Preview"));
    connect(previewDlgBut, &QPushButton::clicked, this, &PrintOptionsPage::onShowPreviewDlg);

    QVBoxLayout *l = new QVBoxLayout(pageLayoutBox);
    l->addWidget(pageSetupDlgBut);
    l->addWidget(previewDlgBut);
}

void PrintOptionsPage::initializePage()
{
    pathEdit->setText(mWizard->getOutputFile());
    patternEdit->setText(mWizard->getFilePattern());
    differentFilesCheckBox->setChecked(mWizard->getDifferentFiles());
    outputTypeCombo->setCurrentIndex(int(mWizard->getOutputType()));
    updateOutputType();
}

bool PrintOptionsPage::validatePage()
{
    if(mWizard->getOutputType() != Print::Native)
    {
        //Check files
        const QString path = pathEdit->text();
        if(path.isEmpty())
        {
            return false;
        }

        const QString pattern = patternEdit->text();
        if(pattern.isEmpty())
        {
            return false;
        }

        mWizard->setOutputFile(QDir::fromNativeSeparators(path));
        mWizard->setFilePattern(pattern);
        mWizard->setDifferentFiles(differentFilesCheckBox->isChecked());
    }

    //Start task
    mWizard->startPrintTask();

    return true;
}

bool PrintOptionsPage::isComplete() const
{
    if(mWizard->getOutputType() == Print::Native)
        return true; //No need to check files

    bool complete = !pathEdit->text().isEmpty();
    if(complete && differentFilesCheckBox->isChecked())
        complete = !patternEdit->text().isEmpty();
    return complete;
}

void PrintOptionsPage::onChooseFile()
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
        switch (mWizard->getOutputType())
        {
        case Print::Pdf:
            ext = QStringLiteral(".pdf");
            fullName = FileFormats::tr(FileFormats::pdfFile);
            break;
        case Print::Svg:
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

void PrintOptionsPage::updateDifferentFiles()
{
    //Pattern is applicable only if printing multiple files
    patternEdit->setEnabled(differentFilesCheckBox->isChecked());

    //If pathEdit contains a file but user checks 'Different Files'
    //We go up to file directory and use that

    QString path = pathEdit->text();
    if(path.isEmpty())
        return;

    QString ext;
    switch (mWizard->getOutputType())
    {
    case Print::Pdf:
        ext = QStringLiteral(".pdf");
        break;
    case Print::Svg:
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

void PrintOptionsPage::onOpenPageSetup()
{
    QPrinter *printer = mWizard->getPrinter();
    if(!printer)
        return;

    if(printer->outputFormat() == QPrinter::NativeFormat)
    {
        //Native dialog for native printer
        OwningQPointer<QPrintDialog> dlg = new QPrintDialog(printer, this);
        dlg->exec();
    }
    else
    {
        //Custom dialog for PDF printer
        OwningQPointer<CustomPageSetupDlg> dlg = new CustomPageSetupDlg(this);

        QPageLayout pageLay = printer->pageLayout();
        dlg->setPageSize(pageLay.pageSize());
        dlg->setPageOrient(pageLay.orientation());
        if(dlg->exec() != QDialog::Accepted || !dlg)
            return;

        pageLay.setPageSize(dlg->getPageSize());
        pageLay.setOrientation(dlg->getPageOrient());
        printer->setPageLayout(pageLay);
    }
}

void PrintOptionsPage::onShowPreviewDlg()
{
    QPrinter *printer = mWizard->getPrinter();

    if(QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
    {
        OwningQPointer<QPrintPreviewDialog> dlg = new QPrintPreviewDialog(printer, this);
        connect(dlg, &QPrintPreviewDialog::paintRequested, this, [this](QPrinter *printer_)
                {
                    //FIXME
                    PrintWorker worker(mWizard->getDb(), this);
                    worker.setPrinter(printer_);
                    worker.setOutputType(Print::Native);
                    worker.setSelection(mWizard->getSelectionModel());
                    worker.run();
                });
        dlg->exec();
    }

    OwningQPointer<ScenePrintPreviewDlg> dlg = new ScenePrintPreviewDlg(this);

    SceneSelectionModel *m = mWizard->getSelectionModel();
    m->startIteration();
    SceneSelectionModel::Entry entry = m->getNextEntry();

    LineGraphScene scene(mWizard->getDb());
    scene.loadGraph(entry.objectId, entry.type);

    dlg->setSourceScene(&scene);
    dlg->setPrinter(printer);

    dlg->exec();

    if(!dlg)
        return;

    if(printer && printer->outputFormat() == QPrinter::PdfFormat)
    {
        //Update page layout
        printer->setPageLayout(dlg->getPrinterPageLay());
    }
}

void PrintOptionsPage::updateOutputType()
{
    Print::OutputType type = Print::OutputType(outputTypeCombo->currentIndex());

    fileBox->setEnabled(type != Print::Native);

    pageLayoutBox->setEnabled(type != Print::Svg);
    pageSetupDlgBut->setText(type == Print::Native ? tr("Printer Options") : tr("Page Setup"));

    if(type == Print::Svg)
    {
        //Svg can only be printed in multiple files
        differentFilesCheckBox->setChecked(true);
        differentFilesCheckBox->setEnabled(false);
    }
    else
    {
        differentFilesCheckBox->setEnabled(true);
    }

    QPrinter *printer = mWizard->getPrinter();
    if(printer)
    {
        printer->setOutputFormat(type == Print::Native ? QPrinter::NativeFormat : QPrinter::PdfFormat);
    }

    mWizard->setOutputType(type);

    //Check if new otptions are valid
    emit completeChanged();
}
