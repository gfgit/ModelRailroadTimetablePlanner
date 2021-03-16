#ifndef SEARCHTASK_H
#define SEARCHTASK_H

#ifdef SEARCHBOX_MODE_ASYNC

#include "utils/thread/iquittabletask.h"

#include <QString>
#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>

typedef struct SearchResultItem_ SearchResultItem;
class QRegularExpression;

class SearchTask : public IQuittableTask
{
public:
    SearchTask(QObject *receiver, int limitRows, sqlite3pp::database &db,
               const QStringList& catNames_, const QStringList& catNamesAbbr_);
    ~SearchTask();

    void run() override;

    void setQuery(const QString &query);

private:
    void searchByCat(const QList<int> &categories, QVector<SearchResultItem> &jobs);
    void searchByCatAndNum(const QList<int> &categories, const QString &num, QVector<SearchResultItem> &jobs);
    void searchByNum(const QString &num, QVector<SearchResultItem> &jobs);

private:
    enum QueryType
    {
        Invalid,
        ByCat,
        ByCatAndNum,
        ByNum
    };

    sqlite3pp::database &mDb;
    sqlite3pp::query q_selectJobs;
    QRegularExpression *regExp;

    QString mQuery;
    int limitResultRows;
    QueryType queryType;
    QStringList catNames, catNamesAbbr; //FIXME: store in search engine cache
};

#endif //SEARCHBOX_MODE_ASYNC

#endif // SEARCHTASK_H
