#ifdef ENABLE_USER_QUERY

#include "sqlconsole.h"

#include <QPlainTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QGridLayout>

#include <QTimer>

#include "sqlviewer.h"

SQLConsole::SQLConsole(QWidget *parent) :
    QDialog(parent),
    timer(nullptr)
{
    setWindowTitle(QStringLiteral("SQL Console"));

    edit = new QPlainTextEdit;
    viewer = new SQLViewer(nullptr);
    QPushButton *clearBut = new QPushButton("Clear");
    QPushButton *runBut = new QPushButton("Run");

    intervalSpin = new QSpinBox;
    intervalSpin->setMinimum(0);
    intervalSpin->setMaximum(10);
    intervalSpin->setSuffix(" seconds");

    QGridLayout *lay = new QGridLayout(this);
    lay->addWidget(edit,     0, 0, 1, 3);
    lay->addWidget(runBut,   1, 0, 1, 1);
    lay->addWidget(clearBut, 1, 1, 1, 1);
    lay->addWidget(intervalSpin,  1, 2, 1, 1);
    lay->addWidget(viewer,   2, 0, 1, 3);

    connect(clearBut, &QPushButton::clicked, edit, &QPlainTextEdit::clear);
    connect(runBut, &QPushButton::clicked, this, &SQLConsole::executeQuery);

    connect(intervalSpin, &QSpinBox::editingFinished, this, &SQLConsole::onIntervalChangedUser);

    setMinimumSize(400, 300);
    setModal(false);
}

SQLConsole::~SQLConsole()
{

}

void SQLConsole::setInterval(int secs)
{
    if(secs == 0 && timer)
    {
        timer->stop();
        timer->deleteLater();
        timer = nullptr;
    }
    else if(secs != 0)
    {
        if(!timer)
        {
            timer = new QTimer(this);
            connect(timer, &QTimer::timeout, this, &SQLConsole::timedExec);
        }
        timer->start(secs * 1000);
        intervalSpin->setValue(secs);
    }
}

void SQLConsole::onIntervalChangedUser()
{
    int secs = intervalSpin->value();
    if(timer && timer->interval() == secs)
        return;
    setInterval(secs);
}

bool SQLConsole::executeQuery()
{
    QString sql = edit->toPlainText();
    if(!viewer->prepare(sql.toUtf8()))
        return false;

    viewer->execQuery();
    return true;
}

void SQLConsole::timedExec()
{
    if(!viewer->execQuery())
    {
        setInterval(0); //Abort timer
        return;
    }
    viewer->timedExec();
}

#endif // ENABLE_USER_QUERY
