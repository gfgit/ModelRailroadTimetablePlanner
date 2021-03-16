#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include <QWizardPage>

class QGroupBox;
class QCheckBox;
class QComboBox;
class QSpinBox;
class IOptionsWidget;
class QScrollArea;

class OptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit OptionsPage(QWidget *parent = nullptr);

    virtual void initializePage() override;
    virtual bool validatePage() override;

private slots:
    void updateGeneralCheckBox();
    void setSource(int s);

private:
    void setMode(int m);
    int getMode();

private:
    QGroupBox *generalBox;
    QCheckBox *importOwners;
    QCheckBox *importModels;
    QCheckBox *importRS;
    QSpinBox  *defaultSpeedSpin;
    QComboBox *defaultTypeCombo;

    QGroupBox *specificBox;
    QComboBox *sourceCombo;
    IOptionsWidget *optionsWidget;
    QScrollArea *scrollArea;
};

#endif // OPTIONSPAGE_H
