#include "printoptionspage.h"

#include "printwizard.h"

#include <QVBoxLayout>
#include "printing/helper/view/printeroptionswidget.h"


PrintOptionsPage::PrintOptionsPage(PrintWizard *w, QWidget *parent) :
    QWizardPage (parent),
    mWizard(w)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    optionsWidget = new PrinterOptionsWidget;
    lay->addWidget(optionsWidget);

    connect(optionsWidget, &PrinterOptionsWidget::completeChanged,
            this, &QWizardPage::completeChanged);

    setTitle(tr("Print Options"));

    setCommitPage(true);

    //Change 'Commit' to 'Print' so user understands better
    setButtonText(QWizard::CommitButton, tr("Print"));
}

void PrintOptionsPage::initializePage()
{
    optionsWidget->setPrinter(mWizard->getPrinter());
    optionsWidget->setOptions(mWizard->getPrintOpt());
}

bool PrintOptionsPage::validatePage()
{
    if(!optionsWidget->validateOptions())
        return false;

    //Update options
    mWizard->setPrintOpt(optionsWidget->getOptions());

    //Start task
    mWizard->startPrintTask();

    return true;
}

bool PrintOptionsPage::isComplete() const
{
    return optionsWidget->isComplete();
}
