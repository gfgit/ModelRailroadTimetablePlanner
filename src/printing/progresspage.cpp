#include "progresspage.h"
#include "printwizard.h"

#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

#include "printworker.h"

PrintProgressPage::PrintProgressPage(PrintWizard *w, QWidget *parent) :
    QWizardPage(parent),
    mWizard(w),
    complete(false)
{
    m_label = new QLabel(tr("Printing..."));
    m_progressBar = new QProgressBar;
    m_progressBar->setMinimum(0);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(m_label);
    l->addWidget(m_progressBar);
    setLayout(l);

    setCommitPage(true);
    setFinalPage(true);

    setTitle(tr("Printing"));
}

void PrintProgressPage::initializePage()
{
    complete = false;
    m_progressBar->reset();

    m_worker = new PrintWorker(mWizard->getDb());
    m_worker->setSelection(mWizard->getSelectionModel());
    m_worker->setOutputType(mWizard->getOutputType());
    m_worker->setPrinter(mWizard->getPrinter());
    m_worker->setFileOutput(mWizard->getOutputFile(), mWizard->getDifferentFiles());

    m_progressBar->setMaximum(m_worker->getMaxProgress());

    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(&m_thread, &QThread::started, m_worker, &PrintWorker::doWork);
    connect(m_worker, &PrintWorker::finished, this, &PrintProgressPage::handleFinished);
    connect(m_worker, &PrintWorker::progress, this, &PrintProgressPage::handleProgress);
    connect(m_worker, &PrintWorker::description, this, &PrintProgressPage::handleDescription);

    m_thread.start();
}

bool PrintProgressPage::validatePage()
{
    if(m_thread.isRunning())
    {
        if(!m_thread.wait(2000))
            return false;
    }

    return complete;
}

bool PrintProgressPage::isComplete() const
{
    return complete;
}

void PrintProgressPage::handleFinished()
{
    m_thread.quit();
    m_thread.wait();

    m_label->setText(tr("Completed"));

    complete = true;
    emit completeChanged();
}

void PrintProgressPage::handleProgress(int val)
{
    m_progressBar->setValue(val);
}

void PrintProgressPage::handleDescription(const QString& text)
{
    m_label->setText(QStringLiteral("Printing '%1' ...").arg(text));
}
