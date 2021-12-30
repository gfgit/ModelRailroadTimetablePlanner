#include "spinboxeditorfactory.h"
#include <QSpinBox>

SpinBoxEditorFactory::SpinBoxEditorFactory() :
    m_minVal(0),
    m_maxVal(99)
{

}

QWidget *SpinBoxEditorFactory::createEditor(int /*userType*/, QWidget *parent) const
{
    QSpinBox *spin = new QSpinBox(parent);
    spin->setRange(m_minVal, m_maxVal);
    spin->setPrefix(m_prefix);
    spin->setSuffix(m_suffix);
    spin->setSpecialValueText(m_specialValueText);
    spin->setAlignment(alignment);
    return spin;
}

QByteArray SpinBoxEditorFactory::valuePropertyName(int) const
{
    return "value";
}

void SpinBoxEditorFactory::setRange(int min, int max)
{
    m_minVal = min;
    m_maxVal = max;
}

void SpinBoxEditorFactory::setPrefix(const QString &prefix)
{
    m_prefix = prefix;
}

void SpinBoxEditorFactory::setSuffix(const QString &suffix)
{
    m_suffix = suffix;
}

void SpinBoxEditorFactory::setSpecialValueText(const QString &specialValueText)
{
    m_specialValueText = specialValueText;
}

void SpinBoxEditorFactory::setAlignment(const Qt::Alignment &value)
{
    alignment = value;
}
