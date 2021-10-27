#ifndef PRINTSELECTIONPAGE_H
#define PRINTSELECTIONPAGE_H

#include <QWizardPage>

class PrintWizard;
class QTableView;
class QPushButton;
class QComboBox;
class QLabel;

class PrintSelectionPage : public QWizardPage
{
    Q_OBJECT
public:
    PrintSelectionPage(PrintWizard *w, QWidget *parent = nullptr);

    bool isComplete() const override;

private slots:
    void comboBoxesChanged();
    void updateComboBoxesFromModel();
    void updateSelectionCount();

    void onAddItem();
    void onRemoveItem();

private:
    void setupComboBoxes();

private:
    PrintWizard *mWizard;

    QTableView *view;

    QPushButton *addBut;
    QPushButton *remBut;
    QPushButton *removeAllBut;
    QComboBox *modeCombo;
    QComboBox *typeCombo;

    QLabel *statusLabel;
};

#endif // PRINTSELECTIONPAGE_H
