#ifndef PRINTPROGRESSPAGE_H
#define PRINTPROGRESSPAGE_H

#include <QWizardPage>

class PrintWizard;
class QLabel;
class QProgressBar;

class PrintProgressPage : public QWizardPage
{
    Q_OBJECT
public:
    PrintProgressPage(PrintWizard *w, QWidget *parent = nullptr);

    bool isComplete() const override;

    void handleProgressStart(int max);
    void handleProgress(int val, const QString &text);

private:
    PrintWizard *mWizard;

    QLabel *m_label;
    QProgressBar *m_progressBar;
};

#endif // PRINTPROGRESSPAGE_H
