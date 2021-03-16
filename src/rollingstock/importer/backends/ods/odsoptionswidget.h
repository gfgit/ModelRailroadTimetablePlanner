#ifndef ODSOPTIONSWIDGET_H
#define ODSOPTIONSWIDGET_H

#include "../ioptionswidget.h"

class QSpinBox;

class ODSOptionsWidget : public IOptionsWidget
{
    Q_OBJECT
public:
    explicit ODSOptionsWidget(QWidget *parent = nullptr);

    void loadSettings(const QMap<QString, QVariant> &settings) override;
    void saveSettings(QMap<QString, QVariant> &settings) override;

private:
    QSpinBox *odsFirstRowSpin;
    QSpinBox *odsNumColSpin;
    QSpinBox *odsNameColSpin;
};

#endif // ODSOPTIONSWIDGET_H
