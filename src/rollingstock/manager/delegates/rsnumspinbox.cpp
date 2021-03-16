#include "rsnumspinbox.h"

//TODO: remove
RsNumSpinBox::RsNumSpinBox(QWidget *parent) :
    QSpinBox(parent)
{
    setRange(0, 9999); //from 000-0 to 999-9
}

QValidator::State RsNumSpinBox::validate(QString &input, int &/*pos*/) const
{
    QString s = input;
    s.remove(QChar('-'));
    bool ok = false;
    int val = s.toInt(&ok);
    if(ok && val >= minimum() && val <= maximum())
        return QValidator::Acceptable;
    return QValidator::Invalid;
}

int RsNumSpinBox::valueFromText(const QString &str) const
{
    QString s = str;
    s.remove(QChar('-'));
    bool ok = false;
    int val = s.toInt(&ok);
    if(ok)
        return val;
    return 0;
}

QString RsNumSpinBox::textFromValue(int val) const
{
    QString str = QString::number(val);
    str = str.rightJustified(4, QChar('0'));
    str.insert(3, QChar('-'));
    return str; //XXX-X
}
