#include "rs_utils.h"

#include <QtMath>

QString rs_utils::formatNum(RsType type, int number)
{
    if(type == RsType::Engine)
    {
        return QString::number(number).rightJustified(3, '0');
    }
    else
    {
        //Freight wagons and coaches
        QString str = QStringLiteral("%1").arg(number, 4, 10, QChar('0'));
        str.insert(3, '-'); // 'XXX-X'
        return str;
    }
}

QString rs_utils::formatName(const QString &model, int number, const QString &suffix, RsType type)
{
    QString name = model;
    name.reserve(model.size() + suffix.size() + 10); //1 char separator (' ', '.') + 9 digit max


    int numberLen = int(floor(log10(number))) + 1;

    if(type == RsType::Engine)
    {
        name.append('.'); //'[Name].XXX[Suffix]

        const int count = qMax(5, numberLen);
        for(int i = 0; i < count; i++)
            name.append('0');
        for(int i = 0; i < numberLen; i++)
        {
            int rem = number % 10;
            name[name.size() - i - 1] = QChar('0' + rem);
            number = (number - rem) / 10;
        }
    }
    else
    {
        name.append(' ');

        //Freight wagons and coaches '[Name] XXX-X[Suffix]'
        numberLen++;
        const int count = qMax(5, numberLen);
        for(int i = 0; i < count; i++)
            name.append('0');
        for(int i = 0; i < numberLen; i++)
        {
            if(i == 1)
            {
                name[name.size() - 2] = '-';
                continue;
            }
            int rem = number % 10;
            name[name.size() - i - 1] = QChar('0' + rem);
            number = (number - rem) / 10;
        }
    }

    if(type == RsType::Engine)
    {
        name.append('.');
        name.append(QString::number(number));
    }else{
        name.append(' ');

        //Freight wagons and coaches
        QString str = QStringLiteral("%1").arg(number, 4, 10, QChar('0'));
        str.insert(3, '-'); // 'XXX-X'
        name.append(str);
    }
    name.append(suffix);
    return name;
}

QString rs_utils::formatNameRef(const char *model, int modelSize, int number, const char *suffix, int suffixSize, RsType type)
{
    QByteArray name;
    name.reserve(modelSize + suffixSize + 10); //1 char separator (' ', '.') + 9 digit max
    name.append(model, modelSize);

    int numberLen = int(floor(log10(number))) + 1;

    if(type == RsType::Engine)
    {
        name.append('.'); //'[Name].XXX[Suffix]

        name.append(qMax(3, numberLen), '0');
        for(int i = 0; i < numberLen; i++)
        {
            int rem = number % 10;
            name[name.size() - i - 1] = char('0' + rem);
            number = (number - rem) / 10;
        }
    }
    else
    {
        name.append(' ');

        //Freight wagons and coaches '[Name] XXX-X[Suffix]'
        numberLen++;
        name.append(qMax(5, numberLen), '0');
        for(int i = 0; i < numberLen; i++)
        {
            if(i == 1)
            {
                name[name.size() - 2] = '-';
                continue;
            }
            int rem = number % 10;
            name[name.size() - i - 1] = char('0' + rem);
            number = (number - rem) / 10;
        }
    }
    name.append(suffix, suffixSize);
    return QString::fromUtf8(name);
}
