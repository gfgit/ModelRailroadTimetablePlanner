#ifndef SQLVIEWER_H
#define SQLVIEWER_H

#ifdef ENABLE_USER_QUERY

#include <QWidget>

class QTableView;
class SQLResultModel;

namespace sqlite3pp {
class query;
}

class SQLViewer : public QWidget
{
    Q_OBJECT
public:
    SQLViewer(sqlite3pp::query *q, QWidget *parent = nullptr);
    virtual ~SQLViewer();

    void showErrorMsg(const QString &title, const QString &msg, int err, int extendedErr);

    void setQuery(sqlite3pp::query *query);
    bool prepare(const QByteArray &sql);
public slots:
    bool execQuery();
    void timedExec();
    void resetColor();

    void onError(int err, int extended, const QString& msg);

private:
    SQLResultModel *model;
    sqlite3pp::query *mQuery;
    QTableView *view;
    bool deleteQuery;
};

#endif // ENABLE_USER_QUERY

#endif // SQLVIEWER_H
