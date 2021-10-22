#ifndef PRINTFILEOPTIONSPAGE_H
#define PRINTFILEOPTIONSPAGE_H

#include <QWizardPage>

class PrintWizard;
class QLineEdit;
class QPushButton;
class QComboBox;
class QGroupBox;
class QCheckBox;

class PrintFileOptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    PrintFileOptionsPage(PrintWizard *w, QWidget *parent = nullptr);
    ~PrintFileOptionsPage();

    void initializePage() override;
    bool validatePage() override;
    bool isComplete() const override;
    int nextId() const override;

public slots:
    void onChooseFile();

    void onDifferentFiles();
private:
    void createFilesBox();

private:
    PrintWizard *mWizard;

    QGroupBox *fileBox;
    QCheckBox *differentFilesCheckBox;
    QLineEdit *pathEdit;
    QPushButton *fileBut;

    QComboBox *pageCombo;
};

#endif // PRINTFILEOPTIONSPAGE_H
