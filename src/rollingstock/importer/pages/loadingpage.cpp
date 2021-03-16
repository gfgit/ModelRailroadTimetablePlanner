#include "loadingpage.h"
#include "../rsimportwizard.h"
#include "../backends/loadprogressevent.h"

#include <QProgressBar>
#include <QVBoxLayout>

LoadingPage::LoadingPage(RSImportWizard *w, QWidget *parent) :
    QWizardPage(parent),
    mWizard(w)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    progressBar = new QProgressBar;
    lay->addWidget(progressBar);
}

bool LoadingPage::isComplete() const
{
    return !mWizard->taskRunning();
}

void LoadingPage::handleProgress(int pr, int max)
{
    if(max == LoadProgressEvent::ProgressMaxFinished)
    {
        progressBar->setValue(progressBar->maximum());
        emit completeChanged();
    }
    else
    {
        progressBar->setMaximum(max);
        progressBar->setValue(pr);
    }
}
