#ifndef PRINTOPTIONSPAGE_H
#define PRINTOPTIONSPAGE_H

#include <QWizardPage>

class PrintWizard;
class PrinterOptionsWidget;

class IGraphScene;

class PrintOptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    PrintOptionsPage(PrintWizard *w, QWidget *parent = nullptr);
    ~PrintOptionsPage();

    bool validatePage() override;
    bool isComplete() const override;

    void setupPage();

    void setScene(IGraphScene *scene);

private:
    PrintWizard *mWizard;

    PrinterOptionsWidget *optionsWidget;

    IGraphScene *m_scene;
};

#endif // PRINTOPTIONSPAGE_H
