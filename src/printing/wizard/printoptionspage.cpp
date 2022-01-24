#include "printoptionspage.h"

#include "printwizard.h"

#include <QVBoxLayout>
#include "printing/helper/view/printeroptionswidget.h"

#include "utils/scene/igraphscene.h"


PrintOptionsPage::PrintOptionsPage(PrintWizard *w, QWidget *parent) :
    QWizardPage (parent),
    mWizard(w),
    m_scene(nullptr)
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

PrintOptionsPage::~PrintOptionsPage()
{
    //Reset scene
    setScene(nullptr);
}

bool PrintOptionsPage::validatePage()
{
    if(!optionsWidget->validateOptions())
        return false;

    //Update options
    mWizard->setPrintOpt(optionsWidget->getOptions());
    return true;
}

bool PrintOptionsPage::isComplete() const
{
    return optionsWidget->isComplete();
}

void PrintOptionsPage::setupPage()
{
    optionsWidget->setPrinter(mWizard->getPrinter());
    optionsWidget->setOptions(mWizard->getPrintOpt());
    setScene(mWizard->getFirstScene());
}

void PrintOptionsPage::setScene(IGraphScene *scene)
{
    if(m_scene)
        delete m_scene;
    m_scene = scene;
    optionsWidget->setSourceScene(m_scene);
}
