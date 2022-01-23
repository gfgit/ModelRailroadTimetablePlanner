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
    void updateDifferentFiles();
    void onOpenPageSetup();
    void onShowPreviewDlg();

private:
    void createFilesBox();
    void createPageLayoutBox();

private:
    PrintWizard *mWizard;

    QGroupBox *fileBox;
    QCheckBox *differentFilesCheckBox;
    QLineEdit *pathEdit;
    QLineEdit *patternEdit;
    QPushButton *fileBut;

    QGroupBox *pageLayoutBox;
    QPushButton *pageSetupDlgBut;
    QPushButton *previewDlgBut;

    QComboBox *outputTypeCombo;
};

#endif // PRINTOPTIONSPAGE_H
