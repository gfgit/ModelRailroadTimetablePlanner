/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SEARCHTASK_H
#define SEARCHTASK_H

#ifdef SEARCHBOX_MODE_ASYNC

#include "utils/thread/iquittabletask.h"

#include <QString>
#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>

struct SearchResultItem;
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
