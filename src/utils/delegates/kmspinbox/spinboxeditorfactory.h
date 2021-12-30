#ifndef SPINBOXEDITORFACTORY_H
#define SPINBOXEDITORFACTORY_H

#include <QItemEditorFactory>

class SpinBoxEditorFactory : public QItemEditorFactory
{
public:
    SpinBoxEditorFactory();

    QWidget *createEditor(int userType, QWidget *parent) const override;
    QByteArray valuePropertyName(int) const override;

    void setRange(int min, int max);
    void setPrefix(const QString &prefix);
    void setSuffix(const QString &suffix);
    void setSpecialValueText(const QString &specialValueText);
    void setAlignment(const Qt::Alignment &value);

private:
    int m_minVal;
    int m_maxVal;
    QString m_prefix;
    QString m_suffix;
    QString m_specialValueText;
    Qt::Alignment alignment;
};

#endif // SPINBOXEDITORFACTORY_H
