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
