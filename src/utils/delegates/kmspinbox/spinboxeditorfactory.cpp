/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
