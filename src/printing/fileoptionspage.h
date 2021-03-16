#ifndef PDFOPTIONSPAGE_H
#define PDFOPTIONSPAGE_H

#include <QWizardPage>

class PrintWizard;
class QLineEdit;
class QPushButton;
class QComboBox;
class QGroupBox;
class QCheckBox;

class FileOptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    FileOptionsPage(PrintWizard *w, QWidget *parent = nullptr);
    ~FileOptionsPage();

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

#endif // PDFOPTIONSPAGE_H
