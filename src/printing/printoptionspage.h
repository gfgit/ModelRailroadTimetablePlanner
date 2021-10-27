#ifndef PRINTOPTIONSPAGE_H
#define PRINTOPTIONSPAGE_H

#include <QWizardPage>

class PrintWizard;
class QLineEdit;
class QPushButton;
class QComboBox;
class QGroupBox;
class QCheckBox;

class PrintOptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    PrintOptionsPage(PrintWizard *w, QWidget *parent = nullptr);
    ~PrintOptionsPage();

    void initializePage() override;
    bool validatePage() override;
    bool isComplete() const override;

private slots:
    void updateOutputType();
    void onChooseFile();
    void onDifferentFiles();
    void onOpenPrintDlg();

private:
    void createFilesBox();
    void createPrinterBox();

private:
    PrintWizard *mWizard;

    QGroupBox *fileBox;
    QCheckBox *differentFilesCheckBox;
    QLineEdit *pathEdit;
    QLineEdit *patternEdit;
    QPushButton *fileBut;

    QGroupBox *printerBox;
    QPushButton *printerOptionDlgBut;

    QComboBox *outputTypeCombo;
};

#endif // PRINTOPTIONSPAGE_H
