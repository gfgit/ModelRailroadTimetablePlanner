#include "loadingpage.h"
#include "../backends/loadprogressevent.h"

#include <QProgressBar>
#include <QVBoxLayout>

LoadingPage::LoadingPage(QWidget *parent) :
    QWizardPage(parent),
    m_isComplete(false)
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    progressBar = new QProgressBar;
    lay->addWidget(progressBar);
}

bool LoadingPage::isComplete() const
{
    return m_isComplete;
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

void LoadingPage::setProgressCompleted(bool val)
{
    m_isComplete = val;
    emit completeChanged();
}
