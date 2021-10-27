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

#include "utils/file_format_names.h"
#include <QFileDialog>
#include <QPrintDialog>
#include <QPointer>

#include <QFileInfo>


PrintOptionsPage::PrintOptionsPage(PrintWizard *w, QWidget *parent) :
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
    connect(outputTypeCombo, qOverload<int>(&QComboBox::activated),
            this, &PrintOptionsPage::updateOutputType);

    QLabel *typeLabel = new QLabel(tr("Output Type:"));

    QGridLayout *lay = new QGridLayout(this);
    lay->addWidget(typeLabel, 0, 0);
    lay->addWidget(outputTypeCombo, 0, 1);
    lay->addWidget(fileBox, 1, 0, 1, 2);
    lay->addWidget(printerBox, 2, 0, 1, 2);

    setTitle(tr("Print Options"));

    //Change 'Next' to 'Print' so user understands better
    setButtonText(QWizard::NextButton, tr("Print"));
}

PrintOptionsPage::~PrintOptionsPage()
{

}

void PrintOptionsPage::createFilesBox()
{
    fileBox = new QGroupBox(tr("Files"));

    differentFilesCheckBox = new QCheckBox(tr("Different Files"));
    connect(differentFilesCheckBox, &QCheckBox::toggled,
            this, &PrintOptionsPage::onDifferentFiles);

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

void PrintOptionsPage::createPrinterBox()
{
    printerBox = new QGroupBox(tr("Printer"));

    printerOptionDlgBut = new QPushButton(tr("Open Printer Options"));
    connect(printerOptionDlgBut, &QPushButton::clicked, this, &PrintOptionsPage::onOpenPrintDlg);

    QVBoxLayout *l = new QVBoxLayout(printerBox);
    l->addWidget(printerOptionDlgBut);
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

    emit mWizard->printOptionsChanged();

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

void PrintOptionsPage::onDifferentFiles()
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

void PrintOptionsPage::onOpenPrintDlg()
{
    QPointer<QPrintDialog> dlg = new QPrintDialog(mWizard->getPrinter(), this);
    dlg->exec();
    delete dlg;
}

void PrintOptionsPage::updateOutputType()
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

    //Check if new otptions are valid
    emit completeChanged();
}
