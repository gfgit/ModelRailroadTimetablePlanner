#ifndef LOADFILEPAGE_H
#define LOADFILEPAGE_H

#include <QWizardPage>

class RSImportWizard;
class QProgressBar;

class LoadingPage : public QWizardPage
{
    Q_OBJECT
public:
    LoadingPage(RSImportWizard *w, QWidget *parent = nullptr);

    virtual bool isComplete() const override;

    void handleProgress(int pr, int max);

private:
    RSImportWizard *mWizard;
    QProgressBar *progressBar;
};

#endif // LOADFILEPAGE_H
