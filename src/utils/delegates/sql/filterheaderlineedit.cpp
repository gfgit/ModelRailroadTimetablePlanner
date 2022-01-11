#include "filterheaderlineedit.h"

#include "utils/delegates/sql/pageditemmodel.h" //For NULL filter

#include <QTimerEvent>
#include <QHelpEvent>
#include <QMenu>

FilterHeaderLineEdit::FilterHeaderLineEdit(int col, QWidget *parent) :
    QLineEdit(parent),
    m_column(col),
    m_textTimerId(0),
    m_allowNull(true)
{
    setPlaceholderText(tr("Filter"));
    setClearButtonEnabled(true);

    //Delay text changed until user stops typing
    connect(this, &QLineEdit::textEdited, this, &FilterHeaderLineEdit::startTextTimer);

    //Bypass timer when losing focus or pressing Enter
    connect(this, &QLineEdit::editingFinished, this, &FilterHeaderLineEdit::delayedTimerFinished);

    actionFilterNull = new QAction(tr("Filter #NULL"), this);
    actionFilterNull->setToolTip(tr("Filter all empty cells in this column."));
    connect(actionFilterNull, &QAction::triggered, this, &FilterHeaderLineEdit::setNULLFilter);
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

void FilterHeaderLineEdit::setAllowNull(bool val)
{
    m_allowNull = val;
    actionFilterNull->setVisible(m_allowNull);
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

void FilterHeaderLineEdit::setNULLFilter()
{
    if(!m_allowNull)
        return;
    setText(IPagedItemModel::nullFilterStr);
    delayedTimerFinished(); //Skip timer, emit now
}

bool FilterHeaderLineEdit::event(QEvent *e)
{
    if(e->type() == QEvent::ToolTip)
    {
        //Send tooltip request to FilterHeaderView
        QHelpEvent *ev = static_cast<QHelpEvent *>(e);
        emit tooltipRequested(this, ev->globalPos());
        return true;
    }

    return QLineEdit::event(e);
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

void FilterHeaderLineEdit::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createStandardContextMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QAction *firstAction = menu->actions().value(0, nullptr);
    menu->insertAction(firstAction, actionFilterNull);
    if(firstAction)
        menu->insertSeparator(actionFilterNull);
    menu->popup(e->globalPos());
}
