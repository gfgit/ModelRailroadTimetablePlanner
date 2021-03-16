#ifndef PROGRESSPAGE_H
#define PROGRESSPAGE_H

#include <QWizardPage>

#include <QThread>

class PrintWizard;
class PrintWorker;
class QLabel;
class QProgressBar;

class ProgressPage : public QWizardPage
{
    Q_OBJECT
public:
    ProgressPage(PrintWizard *w, QWidget *parent = nullptr);

    void initializePage() override;
    bool validatePage() override;
    bool isComplete() const override;
public slots:
    void handleFinished();
    void handleProgress(int val);
    void handleDescription(const QString &text);
private:
    PrintWizard *mWizard;

    QLabel *m_label;
    QProgressBar *m_progressBar;

    PrintWorker *m_worker;
    QThread m_thread;
    bool complete;
};

#endif // PROGRESSPAGE_H
