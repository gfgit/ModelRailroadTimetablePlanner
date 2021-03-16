#ifndef SELECTIONPAGE_H
#define SELECTIONPAGE_H

#include <QWizardPage>

class PrintWizard;
class QListView;
class QPushButton;
class CheckProxyModel;

class SelectionPage : public QWizardPage
{
    Q_OBJECT
public:
    SelectionPage(PrintWizard *w, QWidget *parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;
    bool validatePage() override;
    int nextId() const override;

private:
    PrintWizard *mWizard;

    QListView *view;
    QPushButton *selectAllBut;
    QPushButton *selectNoneBut;

    CheckProxyModel *proxyModel;
};

#endif // SELECTIONPAGE_H
