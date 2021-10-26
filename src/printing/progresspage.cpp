#include "progresspage.h"
#include "printwizard.h"

#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include <QPointer>
#include <QMessageBox>

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

    setFinalPage(true);

    setTitle(tr("Printing"));
}

PrintProgressPage::~PrintProgressPage()
{
    m_thread.wait(5000);
    if(m_worker)
        m_worker->deleteLater();
}

void PrintProgressPage::initializePage()
{
    m_worker = new PrintWorker(mWizard->getDb());
    m_worker->moveToThread(&m_thread);

    connect(&m_thread, &QThread::started, m_worker, &PrintWorker::doWork);
    connect(m_worker, &PrintWorker::finished, this, &PrintProgressPage::handleFinished);
    connect(m_worker, &PrintWorker::progress, this, &PrintProgressPage::handleProgress);
    connect(m_worker, &PrintWorker::description, this, &PrintProgressPage::handleDescription);
    connect(m_worker, &PrintWorker::errorOccured, this, &PrintProgressPage::handleError);

    setupWorker();
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

    m_progressBar->setValue(m_progressBar->maximum());
    m_progressBar->setEnabled(false);
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

void PrintProgressPage::handleError(const QString &text)
{
    QPointer<QMessageBox> msgBox = new QMessageBox(this);
    msgBox->setIcon(QMessageBox::Warning);
    auto tryAgainBut = msgBox->addButton(tr("Try again"), QMessageBox::YesRole);
    msgBox->addButton(QMessageBox::Abort);
    msgBox->setDefaultButton(tryAgainBut);
    msgBox->setText(text);
    msgBox->setWindowTitle(tr("Print Error"));

    msgBox->exec();

    if(msgBox)
    {
        if(msgBox->clickedButton() == tryAgainBut)
        {
            //Launch again worker
            setupWorker();
        }
    }

    delete msgBox;
}

void PrintProgressPage::setupWorker()
{
    m_thread.wait();

    complete = false;
    emit completeChanged();

    m_progressBar->reset();

    m_worker->setSelection(mWizard->getSelectionModel());
    m_worker->setOutputType(mWizard->getOutputType());
    m_worker->setPrinter(mWizard->getPrinter());
    m_worker->setFileOutput(mWizard->getOutputFile(), mWizard->getDifferentFiles());
    m_worker->setFilePattern(mWizard->getFilePattern());

    m_progressBar->setMaximum(m_worker->getMaxProgress());

    m_thread.start();
}
