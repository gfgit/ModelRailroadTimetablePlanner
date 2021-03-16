#ifndef CHOOSEFILEPAGE_H
#define CHOOSEFILEPAGE_H

#include <QWizardPage>

class QLineEdit;
class QPushButton;

class ChooseFilePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit ChooseFilePage(QWidget *parent = nullptr);

    bool isComplete() const override;
    bool validatePage() override;
    void initializePage() override;

private slots:
    void onChoose();

private:
    QLineEdit *pathEdit;
    QPushButton *chooseBut;
};

#endif // CHOOSEFILEPAGE_H
