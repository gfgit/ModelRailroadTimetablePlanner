#ifndef RSNUMSPINBOX_H
#define RSNUMSPINBOX_H

#include <QSpinBox>

class RsNumSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit RsNumSpinBox(QWidget *parent = nullptr);

    virtual QValidator::State validate(QString &input, int &pos) const override;

protected:
    int valueFromText(const QString& text) const override;
    QString textFromValue(int val) const override;
};

#endif // RSNUMSPINBOX_H
