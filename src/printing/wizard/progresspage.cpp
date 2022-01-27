#include "progresspage.h"
#include "printwizard.h"

#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

PrintProgressPage::PrintProgressPage(PrintWizard *w, QWidget *parent) :
    QWizardPage(parent),
    mWizard(w)
{
    m_label = new QLabel(tr("Printing..."));
    m_progressBar = new QProgressBar;
    m_progressBar->setMinimum(0);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(m_label);
    l->addWidget(m_progressBar);
    setLayout(l);

    setFinalPage(true);

    setTitle(tr("Printing"));
}

bool PrintProgressPage::isComplete() const
{
    return !mWizard->taskRunning();
}

void PrintProgressPage::handleProgressStart(int max)
{
    //We are starting new progress
    //Enable progress bar and set maximum
    m_progressBar->setMaximum(max);
    m_progressBar->setEnabled(true);
    emit completeChanged();
}

void PrintProgressPage::handleProgress(int val, const QString& text)
{
    const bool finished = val < 0;

    if(val == m_progressBar->maximum() && !finished)
        m_progressBar->setMaximum(val + 1); //Increase maximum

    m_label->setText(text);
    if(finished)
    {
        //Set progress to max and set disabled
        m_progressBar->setValue(m_progressBar->maximum());
        m_progressBar->setEnabled(false);
        emit completeChanged();
    }
    else
    {
        m_progressBar->setValue(val);
    }
}
