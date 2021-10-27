#include "fileoptionspage.h"

#include "printwizard.h"

#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>

#include <QVBoxLayout>
#include <QGridLayout>

#include "utils/file_format_names.h"
#include <QFileDialog>
#include <QPrintDialog>
#include <QPointer>

#include <QFileInfo>


PrintFileOptionsPage::PrintFileOptionsPage(PrintWizard *w, QWidget *parent) :
    QWizardPage (parent),
    mWizard(w)
{
    createFilesBox();
    createPrinterBox();

    outputTypeCombo = new QComboBox;
    QStringList items;
    items.reserve(int(Print::NTypes));
    for(int i = 0; i < int(Print::NTypes); i++)
        items.append(Print::getOutputTypeName(Print::OutputType(i)));
    outputTypeCombo->addItems(items);
    connect(outputTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PrintFileOptionsPage::onOutputTypeChanged);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->addWidget(fileBox);
    lay->addWidget(printerBox);

    setTitle(tr("Print Options"));
}

PrintFileOptionsPage::~PrintFileOptionsPage()
{

}

void PrintFileOptionsPage::createFilesBox()
{
    fileBox = new QGroupBox(tr("Files"));

    differentFilesCheckBox = new QCheckBox(tr("Different Files"));
    connect(differentFilesCheckBox, &QCheckBox::toggled,
            this, &PrintFileOptionsPage::onDifferentFiles);

    pathEdit = new QLineEdit;
    connect(pathEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);

    patternEdit = new QLineEdit;
    connect(patternEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    patternEdit->setEnabled(false); //Initially different files is not checked

    fileBut = new QPushButton(tr("Choose"));
    connect(fileBut, &QPushButton::clicked, this, &PrintFileOptionsPage::onChooseFile);

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

void PrintFileOptionsPage::createPrinterBox()
{
    printerBox = new QGroupBox(tr("Printer"));

    printerOptionDlgBut = new QPushButton(tr("Open Printer Options"));
    connect(printerOptionDlgBut, &QPushButton::clicked, this, &PrintFileOptionsPage::onOpenPrintDlg);

    QVBoxLayout *l = new QVBoxLayout(printerBox);
    l->addWidget(printerOptionDlgBut);
}

void PrintFileOptionsPage::initializePage()
{
    pathEdit->setText(mWizard->getOutputFile());
    patternEdit->setText(mWizard->getFilePattern());
    differentFilesCheckBox->setChecked(mWizard->getDifferentFiles());
    outputTypeCombo->setCurrentIndex(int(mWizard->getOutputType()));
}

bool PrintFileOptionsPage::validatePage()
{
    if(mWizard->getOutputType() == Print::Native)
        return true;

    const QString path = pathEdit->text();
    if(path.isEmpty())
    {
        return false;
    }

    mWizard->setOutputFile(QDir::fromNativeSeparators(path));
    mWizard->setFilePattern(patternEdit->text());
    mWizard->setDifferentFiles(differentFilesCheckBox->isChecked());

    return true;
}

bool PrintFileOptionsPage::isComplete() const
{
    if(mWizard->getOutputType() == Print::Native)
        return true; //No need to check files

    bool complete = !pathEdit->text().isEmpty();
    if(complete && differentFilesCheckBox->isChecked())
        complete = !patternEdit->text().isEmpty();
    return complete;
}

void PrintFileOptionsPage::onChooseFile()
{
    QString path;
    if(differentFilesCheckBox->isChecked())
    {
        path = QFileDialog::getExistingDirectory(this,
                                                 tr("Choose Folder"),
                                                 pathEdit->text());

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
                                            pathEdit->text(),
                                            fullName);
        if(path.isEmpty()) //User canceled dialog
            return;

        if(!path.endsWith(ext))
            path.append(ext);
    }

    pathEdit->setText(QDir::fromNativeSeparators(path));
}

void PrintFileOptionsPage::onDifferentFiles()
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

void PrintFileOptionsPage::onOpenPrintDlg()
{
    QPointer<QPrintDialog> dlg = new QPrintDialog(mWizard->getPrinter(), this);
    dlg->exec();
    delete dlg;
}

int PrintFileOptionsPage::nextId() const
{
    return 3; //Go to ProgressPage
}

void PrintFileOptionsPage::onOutputTypeChanged()
{
    Print::OutputType type = Print::OutputType(outputTypeCombo->currentIndex());

    fileBox->setEnabled(type != Print::Native);
    printerBox->setEnabled(type == Print::Native);

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

    mWizard->setOutputType(type);
}
