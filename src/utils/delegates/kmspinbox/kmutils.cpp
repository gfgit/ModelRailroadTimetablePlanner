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

#include "kmutils.h"

#include <QtMath>

QString utils::kmNumToText(int kmInMeters)
{
    //Add last digit and '+', at least 5 char (X+XXX)
    int numberLen = qMax(5, int(floor(log10(kmInMeters))) + 2);
    QString str(numberLen, QChar('0'));

    for(int i = 0; i < numberLen; i++)
    {
        if(i == 3)
        {
            str[str.size() - 4] = '+';
            continue;
        }
        int rem = kmInMeters % 10;
        str[str.size() - i - 1] = QChar('0' + rem);
        kmInMeters = (kmInMeters - rem) / 10;
    }
    return str;
}

int utils::kmNumFromTextInMeters(const QString &str)
{
    int kmInMeters = 0;
    for(int i = 0; i < str.size(); i++)
    {
        QChar ch = str.at(i);
        if(ch.isDigit())
        {
            kmInMeters *= 10;
            kmInMeters += ch.digitValue();
        }
    }
    return kmInMeters;
}
