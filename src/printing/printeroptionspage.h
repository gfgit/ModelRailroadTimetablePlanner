#ifndef PRINTEROPTIONS_H
#define PRINTEROPTIONS_H

#include <QWizardPage>

class PrintWizard;
class QPushButton;

class PrinterOptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    PrinterOptionsPage(PrintWizard *w, QWidget *parent = nullptr);

public slots:
    void onOpenDialog();

private:
    PrintWizard *mWizard;

    QPushButton *optionsBut;
};

#endif // PRINTEROPTIONS_H
