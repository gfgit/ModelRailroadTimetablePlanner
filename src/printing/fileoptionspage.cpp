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

#include <QFileDialog>
#include "utils/file_format_names.h"

#include <QPrinter>

#include <QFileInfo>

FileOptionsPage::FileOptionsPage(PrintWizard *w, QWidget *parent) :
    QWizardPage (parent),
    mWizard(w)
{
    createFilesBox();
    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(fileBox);
    setLayout(l);

    setTitle(tr("File options"));
}

FileOptionsPage::~FileOptionsPage()
{

}

void FileOptionsPage::createFilesBox()
{
    fileBox = new QGroupBox(tr("Files"));

    differentFilesCheckBox = new QCheckBox(tr("Different Files"));
    connect(differentFilesCheckBox, &QCheckBox::toggled,
            this, &FileOptionsPage::onDifferentFiles);

    pathEdit = new QLineEdit;
    connect(pathEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);

    fileBut = new QPushButton(tr("Choose"));
    connect(fileBut, &QPushButton::clicked, this, &FileOptionsPage::onChooseFile);

    QLabel *label = new QLabel(tr("File(s)"));

    QGridLayout *l = new QGridLayout;
    l->addWidget(differentFilesCheckBox, 0, 0, 1, 2);
    l->addWidget(label, 1, 0, 1, 2);
    l->addWidget(pathEdit, 2, 0);
    l->addWidget(fileBut, 2, 1);
    fileBox->setLayout(l);
}

void FileOptionsPage::initializePage()
{
    pathEdit->setText(mWizard->fileOutput);
    if(mWizard->type == Print::Svg)
    {
        //Svg can only be printed in multiple files
        differentFilesCheckBox->setChecked(true);
        differentFilesCheckBox->setDisabled(true);
    }
    else
    {
        differentFilesCheckBox->setChecked(mWizard->differentFiles);
        differentFilesCheckBox->setEnabled(true);
    }
}

bool FileOptionsPage::validatePage()
{
    QString path = pathEdit->text();
    if(path.isEmpty())
    {
        return false;
    }

    mWizard->fileOutput = QDir::fromNativeSeparators(path);
    mWizard->differentFiles = differentFilesCheckBox->isChecked();

    return true;
}

bool FileOptionsPage::isComplete() const
{
    return !pathEdit->text().isEmpty();
}

void FileOptionsPage::onChooseFile()
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
        switch (mWizard->type)
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

void FileOptionsPage::onDifferentFiles()
{
    //If pathEdit contains a file but user checks 'Different Files'
    //We go up to file directory and use that

    QString path = pathEdit->text();
    if(path.isEmpty())
        return;

    QString ext;
    switch (mWizard->type)
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

int FileOptionsPage::nextId() const
{
    return 3; //Go to ProgressPage
}
