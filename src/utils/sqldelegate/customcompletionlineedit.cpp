#include "customcompletionlineedit.h"
#include "isqlfkmatchmodel.h"

#include <QTreeView>
#include <QHeaderView>

#include <QTimerEvent>
#include <QKeyEvent>

#include <QDebug>

CustomCompletionLineEdit::CustomCompletionLineEdit(ISqlFKMatchModel *m, QWidget *parent) :
    QLineEdit(parent),
    popup(nullptr),
    model(nullptr),
    dataId(0),
    suggestionsTimerId(0)
{
    popup = new QTreeView;
    popup->setWindowFlags(Qt::Popup);
    popup->setFocusPolicy(Qt::NoFocus);
    popup->setFocusProxy(this);
    popup->setMouseTracking(true);
    popup->header()->hide();

#ifdef PRINT_DBG_MSG
    popup->setObjectName(QStringLiteral("Popup %1 (%2)").arg(qintptr(popup)).arg(qintptr(this)));
    setObjectName(QStringLiteral("CustomCompletionLineEdit (%1)").arg(qintptr(this)));
#endif

    popup->setUniformRowHeights(true);
    popup->setRootIsDecorated(false);
    popup->setEditTriggers(QAbstractItemView::NoEditTriggers);
    popup->setSelectionBehavior(QAbstractItemView::SelectRows);
    popup->setFrameStyle(QFrame::Box | QFrame::Plain);
    popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    popup->installEventFilter(this);

    connect(popup, &QAbstractItemView::clicked, this, &CustomCompletionLineEdit::doneCompletion);
    connect(this, &QLineEdit::textEdited, this, &CustomCompletionLineEdit::startSuggestionsTimer);

    setModel(m);
}

CustomCompletionLineEdit::~CustomCompletionLineEdit()
{
    if(suggestionsTimerId)
    {
        killTimer(suggestionsTimerId);
        suggestionsTimerId = 0;
    }
    delete popup;
}

void CustomCompletionLineEdit::showPopup()
{
    if(isReadOnly() || popup->isVisible())
        return;

    popup->move(mapToGlobal(QPoint(0, height())));
    popup->resize(width(), height() * 4);
    popup->setFocus();
    popup->show();
    if(model)
        model->refreshData();
}

bool CustomCompletionLineEdit::getData(db_id &idOut, QString& nameOut) const
{
    idOut = dataId;
    nameOut = text();

    return dataId != 0;
}

void CustomCompletionLineEdit::setData(db_id id, const QString &name)
{
    if(model && id && name.isEmpty())
        setText(model->getName(id));
    else
        setText(name);

    if(id == dataId)
        return;

    dataId = id;
    emit dataIdChanged(dataId);
}

void CustomCompletionLineEdit::setModel(ISqlFKMatchModel *m)
{
    if(model)
        disconnect(model, &ISqlFKMatchModel::resultsReady, this, &CustomCompletionLineEdit::resultsReady);

    model = m;
    popup->setModel(model);
    connect(model, &ISqlFKMatchModel::resultsReady, this, &CustomCompletionLineEdit::resultsReady);

    //Force reloading of current entry
    setData(dataId, QString());

    if(popup->isVisible())
    {
        model->autoSuggest(text());
        model->refreshData();
    }
}

void CustomCompletionLineEdit::timerEvent(QTimerEvent *e)
{
    if(suggestionsTimerId && e->timerId() == suggestionsTimerId)
    {
        killTimer(suggestionsTimerId);
        suggestionsTimerId = 0;

        if(model)
        {
            model->autoSuggest(text());
            showPopup();
        }

        return;
    }

    QLineEdit::timerEvent(e);
}

void CustomCompletionLineEdit::mousePressEvent(QMouseEvent *e)
{
    showPopup();
    QLineEdit::mousePressEvent(e);
}

bool CustomCompletionLineEdit::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj != popup)
        return false;

    if (ev->type() == QEvent::MouseButtonPress)
    {
        popup->hide();
        if(model)
            model->clearCache();
        setFocus();
        return true;
    }

    if (ev->type() == QEvent::KeyPress)
    {
        bool consumed = false;
        int key = static_cast<QKeyEvent*>(ev)->key();
        switch (key)
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            doneCompletion(popup->currentIndex());
            consumed = true;
            break;

        case Qt::Key_Escape:
            setFocus();
            popup->hide();
            if(model)
                model->clearCache();
            consumed = true;
            break;

        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            break;

        default:
            setFocus();
            event(ev);
            popup->hide();
            if(model)
                model->clearCache();
            break;
        }

        return consumed;
    }

    return false;
}

void CustomCompletionLineEdit::doneCompletion(const QModelIndex& idx)
{
    if(suggestionsTimerId)
    {
        killTimer(suggestionsTimerId);
        suggestionsTimerId = 0;
    }

    if(model)
    {
        if(idx.row() >= 0 && !model->isEmptyRow(idx.row()) && !model->isEllipsesRow(idx.row()))
        {
            setData(model->getIdAtRow(idx.row()), model->getNameAtRow(idx.row()));
        }
        else
        {
            setData(0, QString());
        }

        model->clearCache();
    }

    popup->hide();
    clearFocus();

    emit completionDone(this);
}

void CustomCompletionLineEdit::startSuggestionsTimer()
{
    if(suggestionsTimerId)
        killTimer(suggestionsTimerId);
    suggestionsTimerId = startTimer(250);
}

void CustomCompletionLineEdit::resultsReady(bool forceFirst)
{
    resizeColumnToContents();
    selectFirstIndexOrNone(forceFirst);
}

void CustomCompletionLineEdit::resizeColumnToContents()
{
    if(!model)
        return;

    const int colCount = model->columnCount();
    for(int i = 0; i < colCount; i++)
        popup->resizeColumnToContents(i);
}

void CustomCompletionLineEdit::selectFirstIndexOrNone(bool forceFirst)
{
    QModelIndex idx; //Invalid index (no item chosen, empty text)
    if(model && (dataId || forceFirst))
        idx = model->index(0, 0); //Select the chosen item
    popup->setCurrentIndex(idx);
}

void CustomCompletionLineEdit::setData_slot(db_id id)
{
    setData(id, QString());
}
