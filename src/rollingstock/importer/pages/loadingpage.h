#ifndef LOADFILEPAGE_H
#define LOADFILEPAGE_H

#include <QWizardPage>

class QProgressBar;

class LoadingPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit LoadingPage(QWidget *parent = nullptr);

    virtual bool isComplete() const override;

    void handleProgress(int pr, int max);
    void setProgressCompleted(bool val);

private:
    QProgressBar *progressBar;
    bool m_isComplete;
};

#endif // LOADFILEPAGE_H
