#ifndef PRINTOPTIONSPAGE_H
#define PRINTOPTIONSPAGE_H

#include <QWizardPage>

class PrintWizard;
class PrinterOptionsWidget;

class PrintOptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    PrintOptionsPage(PrintWizard *w, QWidget *parent = nullptr);

    void initializePage() override;
    bool validatePage() override;
    bool isComplete() const override;

private:
    PrintWizard *mWizard;

    PrinterOptionsWidget *optionsWidget;
};

#endif // PRINTOPTIONSPAGE_H
