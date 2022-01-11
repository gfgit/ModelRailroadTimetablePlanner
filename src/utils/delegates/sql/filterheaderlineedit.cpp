#include "filterheaderlineedit.h"

#include <QTimerEvent>

FilterHeaderLineEdit::FilterHeaderLineEdit(int col, QWidget *parent) :
    QLineEdit(parent),
    m_column(col),
    m_textTimerId(0)
{
    setPlaceholderText(tr("Filter"));
    setClearButtonEnabled(true);

    //Delay text changed until user stops typing
    connect(this, &QLineEdit::textEdited, this, &FilterHeaderLineEdit::startTextTimer);

    //Bypass timer when losing focus or pressing Enter
    connect(this, &QLineEdit::editingFinished, this, &FilterHeaderLineEdit::delayedTimerFinished);
}

FilterHeaderLineEdit::~FilterHeaderLineEdit()
{
    stopTextTimer();
}

void FilterHeaderLineEdit::updateTextWithoutEmitting(const QString &str)
{
    setText(str);
    lastValue = str;
    stopTextTimer();
}

void FilterHeaderLineEdit::startTextTimer()
{
    stopTextTimer();
    m_textTimerId = startTimer(700);
}

void FilterHeaderLineEdit::stopTextTimer()
{
    if(m_textTimerId)
    {
        killTimer(m_textTimerId);
        m_textTimerId = 0;
    }
}

void FilterHeaderLineEdit::delayedTimerFinished()
{
    stopTextTimer();
    if(lastValue != text())
    {
        lastValue = text();
        emit delayedTextChanged(this, lastValue);
    }
}

void FilterHeaderLineEdit::timerEvent(QTimerEvent *e)
{
    if(e->timerId() == m_textTimerId)
    {
        delayedTimerFinished();
        return;
    }

    QLineEdit::timerEvent(e);
}
