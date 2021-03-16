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
