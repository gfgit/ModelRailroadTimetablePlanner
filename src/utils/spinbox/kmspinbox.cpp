#include "kmspinbox.h"
#include "utils/kmutils.h"

#include <QKeyEvent>

#include <QLineEdit>

#include <QDebug>

KmSpinBox::KmSpinBox(QWidget *parent) :
    QSpinBox(parent),
    currentSection(KmSection)
{
    setRange(0, 9999 * 1000 + 999); //9999 km + 999 meters
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    connect(lineEdit(), &QLineEdit::cursorPositionChanged, this, &KmSpinBox::cursorPosChanged);
}

void KmSpinBox::focusInEvent(QFocusEvent *e)
{
    QSpinBox::focusInEvent(e);
    setCurrentSection(KmSection);
}

void KmSpinBox::keyPressEvent(QKeyEvent *e)
{
    bool plusPressed = false;
    if(e->key() == Qt::Key_Plus || e->text().contains('+'))
        plusPressed = true;
    QSpinBox::keyPressEvent(e);
    if(plusPressed && currentSection == KmSection)
        setCurrentSection(MetersSection);
}

bool KmSpinBox::focusNextPrevChild(bool next)
{
    if((currentSection == MetersSection) == next)
    {
        //Before KmSection or after MetersSection is out of the widget
        return QSpinBox::focusNextPrevChild(next);
    }
    setCurrentSection(currentSection == KmSection ? MetersSection : KmSection);
    return true;
}

QValidator::State KmSpinBox::validate(QString &input, int &pos) const
{
    int plusPos = -1;
    int val = 0;
    int firstNonBlank = 0;
    int i = 0;
    bool empty = true;

    QString copy = stripped(input, &pos);

    for(; i < copy.size(); i++)
    {
        QChar ch = copy.at(i);
        if(ch.isSpace())
        {
            if(!empty)
                break;
            firstNonBlank++;
            continue;
        }
        if(ch.isDigit())
        {
            val *= 10;
            val += ch.digitValue();
            empty = false;
            continue;
        }
        if(ch != '+' || plusPos != -1) //Not '+' or digit or multiple '+'
            return QValidator::Invalid;
        plusPos = i;
    }

    if(empty || val > maximum() || plusPos == -1)
        return QValidator::Invalid;

    if(plusPos != i - 4)
        return QValidator::Intermediate; //+ is not 3 chars from last

    if(firstNonBlank > 0 && copy.at(firstNonBlank) == '+')
        firstNonBlank--;

    pos -= copy.size() - i - firstNonBlank;
    copy = copy.mid(firstNonBlank, i - firstNonBlank);

    if(copy.at(0) == '+')
    {
        copy.prepend('0'); //Add leading zero
    }
    else if(copy.size() > 1 && copy.at(1) == '+' && !copy.at(0).isDigit())
    {
        copy[0] = '0'; //Replace first char with leading zero
    }

    //FIXME: prefix creates problems parsing
    input = prefix() + copy + suffix();

    return QValidator::Acceptable;
}

int KmSpinBox::valueFromText(const QString &text) const
{
    return utils::kmNumFromTextInMeters(text);
}

QString KmSpinBox::textFromValue(int val) const
{
    return utils::kmNumToText(val);
}

void KmSpinBox::fixup(QString &str) const
{
    if(str.isEmpty() || str.at(0) == '+')
        str.prepend('0');
    int plusPos = str.indexOf('+');
    if(plusPos < 0)
    {
        str.append('+');
        plusPos = str.size() - 1;
    }
    else if(str.indexOf('+', plusPos + 1) >= 0)
    {
        return; //Do not fix if there are multiple '+'
    }

    int nDigitsAfterPlus = str.size() - plusPos - 1;
    if(nDigitsAfterPlus < 3)
    {
        for(int i = nDigitsAfterPlus; i < 3; i++)
        {
            str.append('0');
        }
    }
    else if(nDigitsAfterPlus > 3)
    {
        str.chop(nDigitsAfterPlus - 3);
    }
}

void KmSpinBox::stepBy(int steps)
{
    int val = value();
    if(currentSection == KmSection)
    {
        //If steps are negative apply only if result km part is >= 0
        val += steps * 1000;
        if(val >= 0)
        {
            setValue(val);
        }
    }
    else
    {
        //Keep 0 <= meters <= 999 otherwhise we change also km part
        int meters = val % 1000 + steps;
        if(meters >= 0 && meters <= 999)
            setValue(val + steps);
    }
    setCurrentSection(currentSection);
}

void KmSpinBox::cursorPosChanged(int /*oldPos*/, int newPos)
{
    QString text = lineEdit()->text();
    int plusPos = text.indexOf('+');
    if(plusPos < 0)
    {
        currentSection = KmSection;
        return;
    }
    currentSection = newPos <= plusPos ? KmSection : MetersSection;
}

void KmSpinBox::setCurrentSection(int section)
{
    QLineEdit *edit = lineEdit();
    QString text = edit->text();
    const int plusPos = text.indexOf('+');
    const int prefixSize = prefix().size();
    const int suffixSize = suffix().size();
    if(plusPos < 0)
    {
        currentSection = KmSection;
        return;
    }
    edit->deselect();
    currentSection = section;
    if(currentSection == KmSection)
    {
        //Select after prefix, before '+'
        const int length = plusPos - prefixSize;
        edit->setSelection(prefixSize, length);
    }
    else
    {
        //Select after '+', before suffix
        const int length = text.size() - suffixSize - plusPos - 1;
        edit->setSelection(plusPos + 1, length);
    }
}

QString KmSpinBox::stripped(const QString &t, int *pos) const
{
    QStringRef text(&t);
    if (specialValueText().size() == 0 || text != specialValueText())
    {
        int from = 0;
        int size = text.size();
        bool changed = false;

        if(prefix().size() && text.startsWith(prefix()))
        {
            from += prefix().size();
            size -= from;
            changed = true;
        }
        if(suffix().size() && text.endsWith(suffix()))
        {
            size -= suffix().size();
            changed = true;
        }
        if (changed)
            text = text.mid(from, size);
    }
    const int s = text.size();
    text = text.trimmed();
    if (pos)
        (*pos) -= (s - text.size());
    return text.toString();
}
