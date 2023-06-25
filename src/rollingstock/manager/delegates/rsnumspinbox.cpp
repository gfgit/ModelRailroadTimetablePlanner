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

#include "rsnumspinbox.h"

// TODO: remove
RsNumSpinBox::RsNumSpinBox(QWidget *parent) :
    QSpinBox(parent)
{
    setRange(0, 9999); // from 000-0 to 999-9
}

QValidator::State RsNumSpinBox::validate(QString &input, int & /*pos*/) const
{
    QString s = input;
    s.remove(QChar('-'));
    bool ok = false;
    int val = s.toInt(&ok);
    if (ok && val >= minimum() && val <= maximum())
        return QValidator::Acceptable;
    return QValidator::Invalid;
}

int RsNumSpinBox::valueFromText(const QString &str) const
{
    QString s = str;
    s.remove(QChar('-'));
    bool ok = false;
    int val = s.toInt(&ok);
    if (ok)
        return val;
    return 0;
}

QString RsNumSpinBox::textFromValue(int val) const
{
    QString str = QString::number(val);
    str         = str.rightJustified(4, QChar('0'));
    str.insert(3, QChar('-'));
    return str; // XXX-X
}
