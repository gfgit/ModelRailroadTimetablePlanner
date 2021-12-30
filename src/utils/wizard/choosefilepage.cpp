#include "choosefilepage.h"

#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

#include <QFileDialog>
#include <QMessageBox>
#include "utils/owningqpointer.h"

#include <QDir>

#include "utils/files/file_format_names.h"

ChooseFilePage::ChooseFilePage(QWidget *parent) :
    QWizardPage(parent)
{
    QHBoxLayout *lay = new QHBoxLayout(this);

    pathEdit = new QLineEdit;
    pathEdit->setPlaceholderText(tr("Insert path here or click 'Choose' button"));
    connect(pathEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    lay->addWidget(pathEdit);

    chooseBut = new QPushButton(tr("Choose"));
    connect(chooseBut, &QPushButton::clicked, this, &ChooseFilePage::onChoose);
    lay->addWidget(chooseBut);

    setTitle(tr("Choose file"));
    setSubTitle(tr("Choose a file to import in selected format"));

    //Prevent user from going back to this page and change file.
    //If user wants to change file he has to Cancel the wizard and start it again
    setCommitPage(true);
}

bool ChooseFilePage::isComplete() const
{
    return !pathEdit->text().isEmpty();
}

void ChooseFilePage::initializePage()
{
    //HACK: I don't like the 'Commit' button. This hack makes it similar to 'Next' button
    setButtonText(QWizard::CommitButton, wizard()->buttonText(QWizard::NextButton));
}

void ChooseFilePage::setFileDlgOptions(const QString &dlgTitle, const QStringList &fileFormats)
{
    fileDlgTitle = dlgTitle;
    fileDlgFormats = fileFormats;
}

bool ChooseFilePage::validatePage()
{
    QString fileName = pathEdit->text();

    QFileInfo f(fileName);
    if(f.exists() && f.isFile())
    {
        emit fileChosen(fileName);
        return true;
    }

    QMessageBox::warning(this, tr("File doesn't exist"),
                         tr("Could not find file '%1'").arg(fileName));
    return false;
}

void ChooseFilePage::onChoose()
{
    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, fileDlgTitle, pathEdit->text());
    dlg->setFileMode(QFileDialog::ExistingFile);
    dlg->setAcceptMode(QFileDialog::AcceptOpen);
    dlg->setNameFilters(fileDlgFormats);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();
    pathEdit->setText(QDir::toNativeSeparators(fileName));
}
