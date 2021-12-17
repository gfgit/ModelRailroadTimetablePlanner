#include "choosefilepage.h"

#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

#include <QFileDialog>
#include <QMessageBox>
#include "utils/owningqpointer.h"

#include <QDir>

#include "../rsimportwizard.h"
#include "../rsimportstrings.h"

#include "utils/file_format_names.h"

ChooseFilePage::ChooseFilePage(QWidget *parent) :
    QWizardPage(parent)
{
    QHBoxLayout *lay = new QHBoxLayout(this);

    pathEdit = new QLineEdit;
    pathEdit->setPlaceholderText(RsImportStrings::tr("Insert path here or click 'Choose' button"));
    connect(pathEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
    lay->addWidget(pathEdit);

    chooseBut = new QPushButton(RsImportStrings::tr("Choose"));
    connect(chooseBut, &QPushButton::clicked, this, &ChooseFilePage::onChoose);
    lay->addWidget(chooseBut);

    setTitle(RsImportStrings::tr("Choose file"));
    setSubTitle(RsImportStrings::tr("Choose a file to import in *.ods format"));

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

bool ChooseFilePage::validatePage()
{
    QString fileName = pathEdit->text();

    QFileInfo f(fileName);
    if(f.exists() && f.isFile())
    {
        RSImportWizard *w = static_cast<RSImportWizard *>(wizard());
        w->startLoadTask(fileName);
        return true;
    }

    QMessageBox::warning(this,
                         RsImportStrings::tr("File doesn't exist"),
                         RsImportStrings::tr("Could not find file '%1'")
                         .arg(fileName));
    return false;
}

void ChooseFilePage::onChoose()
{
    RSImportWizard *w = static_cast<RSImportWizard *>(wizard());

    QString title;
    QStringList filters;
    switch (w->getImportSource())
    {
    case RSImportWizard::ImportSource::OdsImport:
    {
        title = RsImportStrings::tr("Open Spreadsheet");
        filters << FileFormats::tr(FileFormats::odsFormat);
        break;
    }
    case RSImportWizard::ImportSource::SQLiteImport:
    {
        title = RsImportStrings::tr("Open Session");
        filters << FileFormats::tr(FileFormats::tttFormat);
        filters << FileFormats::tr(FileFormats::sqliteFormat);
        break;
    }
    default:
        break;
    }
    filters << FileFormats::tr(FileFormats::allFiles);

    OwningQPointer<QFileDialog> dlg = new QFileDialog(this, title, pathEdit->text());
    dlg->setFileMode(QFileDialog::ExistingFile);
    dlg->setAcceptMode(QFileDialog::AcceptOpen);
    dlg->setNameFilters(filters);

    if(dlg->exec() != QDialog::Accepted || !dlg)
        return;

    QString fileName = dlg->selectedUrls().value(0).toLocalFile();
    pathEdit->setText(QDir::toNativeSeparators(fileName));
}
