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

    void setFileDlgOptions(const QString& dlgTitle, const QStringList& fileFormats);

signals:
    void fileChosen(const QString& fileName);

private slots:
    void onChoose();

private:
    QLineEdit *pathEdit;
    QPushButton *chooseBut;

    QString fileDlgTitle;
    QStringList fileDlgFormats;
};

#endif // CHOOSEFILEPAGE_H
