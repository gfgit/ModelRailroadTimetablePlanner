#ifdef ENABLE_USER_QUERY

#include "sqlviewer.h"

#include "app/session.h"

#include "sqlresultmodel.h"
#include <sqlite3pp/sqlite3pp.h>

#include <QTableView>
#include <QMessageBox>
#include <QVBoxLayout>

#include <QTimer>

#include <QDebug>

SQLViewer::SQLViewer(sqlite3pp::query *q, QWidget *parent) :
    QWidget (parent),
    model(nullptr),
    mQuery(nullptr),
    view(nullptr),
    deleteQuery(false)
{
    setQuery(q);
    setWindowTitle(QStringLiteral("SQL Viewer"));
    view = new QTableView(this);
    QVBoxLayout *l = new QVBoxLayout(this);
    l->addWidget(view);

    model = new SQLResultModel(this);
    connect(model, &SQLResultModel::error, this, &SQLViewer::onError);
    resetColor();
    view->setModel(model);
    view->setEditTriggers(QTableView::NoEditTriggers);

    setMinimumSize(300, 200);
}

SQLViewer::~SQLViewer()
{
    if(deleteQuery)
        delete mQuery;
}

bool SQLViewer::execQuery()
{
    bool ret = false;
    {
        mQuery->reset();
        ret = model->initFromQuery(mQuery);
        mQuery->reset();
    }
    return ret;
}

void SQLViewer::timedExec()
{
    QColor c(255, 0, 0, 50);
    model->setBackground(c);
    QTimer::singleShot(300, this, &SQLViewer::resetColor);
}

void SQLViewer::resetColor()
{
    model->setBackground(Qt::white);
}

void SQLViewer::onError(int err, int extended, const QString &msg)
{
    showErrorMsg(tr("Query Error"), msg, err, extended);
}

void SQLViewer::showErrorMsg(const QString& title, const QString& msg, int err, int extendedErr)
{
    QMessageBox::warning(this, title,
                         tr("SQLite Error: %1\n"
                            "Extended: %2\n"
                            "Message: %3")
                         .arg(err)
                         .arg(extendedErr)
                         .arg(msg));
}

void SQLViewer::setQuery(sqlite3pp::query *query)
{
    if(deleteQuery)
        delete mQuery;
    mQuery = query;
    deleteQuery = !mQuery;
    if(!mQuery)
        mQuery = new sqlite3pp::query(Session->m_Db);
}

bool SQLViewer::prepare(const QByteArray& sql)
{
    int ret = mQuery->prepare(sql);
    if(ret != SQLITE_OK)
    {
        showErrorMsg(tr("Preparation Failed"), Session->m_Db.error_msg(), ret, Session->m_Db.extended_error_code());
        return false;
    }
    return true;
}

#endif // ENABLE_USER_QUERY
