#ifndef PRINTSELECTIONPAGE_H
#define PRINTSELECTIONPAGE_H

#include <QWizardPage>

class PrintWizard;
class QListView;
class QPushButton;
class QComboBox;
class QLabel;

class PrintSelectionPage : public QWizardPage
{
    Q_OBJECT
public:
    PrintSelectionPage(PrintWizard *w, QWidget *parent = nullptr);

    bool isComplete() const override;
    int nextId() const override;

private slots:
    void updateComboBoxes();
    void updateSelectionCount();

private:
    void setupComboBoxes();

private:
    PrintWizard *mWizard;

    QListView *view;
    QPushButton *selectNoneBut;
    QComboBox *modeCombo;
    QComboBox *typeCombo;

    QLabel *statusLabel;
};

#endif // PRINTSELECTIONPAGE_H
