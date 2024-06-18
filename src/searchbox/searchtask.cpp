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

#ifdef SEARCHBOX_MODE_ASYNC

#    include "searchtask.h"
#    include "searchresultitem.h"
#    include "searchresultevent.h"

#    include <QRegularExpression>

#    include "app/session.h"

#    include <sqlite3pp/sqlite3pp.h>

SearchTask::SearchTask(QObject *receiver, int limitRows, sqlite3pp::database &db,
                       const QStringList &catNames_, const QStringList &catNamesAbbr_) :
    IQuittableTask(receiver),
    mDb(db),
    q_selectJobs(mDb),
    regExp(nullptr),
    limitResultRows(limitRows),
    queryType(Invalid),
    catNames(catNames_),
    catNamesAbbr(catNamesAbbr_)
{
}

SearchTask::~SearchTask()
{
    if (regExp)
    {
        delete regExp;
        regExp = nullptr;
    }
}

void SearchTask::run()
{
    QList<SearchResultItem> results;

    if (!regExp)
        regExp = new QRegularExpression("(?<name>[^0-9\\s]*)\\s*(?<num>\\d*)");

    QRegularExpressionMatch match = regExp->match(mQuery);
    if (!match.hasMatch())
    {
        sendEvent(new SearchResultEvent(this, results), true);
        return;
    }

    QString name = match.captured("name");
    QString num  = match.captured("num");

    if (wasStopped())
    {
        sendEvent(new SearchResultEvent(this, results), true);
        return;
    }

    if (!name.isEmpty())
    {
        // Find the matching category
        name = name.toUpper();
        QList<int> categories;

        int cat = catNamesAbbr.indexOf(name);

        if (cat == -1) // Retry with full names
            cat = catNames.indexOf(name);

        if (cat != -1)
        {
            categories.append(cat);
        }
        else
        {
            // Retry with full names that start with ... (partial names)
            for (int i = 0; i < catNames.size(); i++)
            {
                if (catNames.at(i).startsWith(name, Qt::CaseInsensitive))
                {
                    categories.append(i); // Don't break, allow multiple categories
                }
            }
        }

        if (categories.isEmpty())
            return; // Failed to find a category

        if (num.isEmpty())
        {
            // It's a category like 'IC'
            // Match all jobs with that category
            searchByCat(categories, results);
        }
        else
        {
            // Category + number
            // Match all jobs beggining with this number and with this category
            searchByCatAndNum(categories, num, results);
        }
    }
    else if (!num.isEmpty())
    {
        // Search all jobs beginning with this number
        searchByNum(num, results);
    }

    sendEvent(new SearchResultEvent(this, results), true);
}

void SearchTask::searchByCat(const QList<int> &categories, QList<SearchResultItem> &jobs)
{
    if (queryType != ByCat)
    {
        q_selectJobs.prepare("SELECT id FROM jobs WHERE category=?1 ORDER BY id LIMIT ?2");
        queryType = ByCat;
    }

    for (const int cat : categories)
    {
        SearchResultItem item;
        item.category = JobCategory(cat);

        q_selectJobs.bind(1, cat);
        q_selectJobs.bind(2, limitResultRows);

        for (auto job : q_selectJobs)
        {
            // Check every 4 items
            if (jobs.size() % 4 == 0 && wasStopped())
                return;

            item.jobId = job.get<db_id>(0);
            jobs.append(item);
        }
        q_selectJobs.reset();
    }
}

void SearchTask::searchByCatAndNum(const QList<int> &categories, const QString &num,
                                   QList<SearchResultItem> &jobs)
{
    if (queryType != ByCatAndNum)
    {
        q_selectJobs.prepare("SELECT id, id LIKE ?2 as job_rank FROM jobs"
                             " WHERE category=?1 AND id LIKE ?3"
                             " ORDER BY job_rank DESC, id ASC LIMIT ?4");
        queryType = ByCatAndNum;
    }

    for (const int cat : categories)
    {
        SearchResultItem item;
        item.category = JobCategory(cat);

        q_selectJobs.bind(1, cat);
        q_selectJobs.bind(2, num + '%');
        q_selectJobs.bind(3, '%' + num + '%');
        q_selectJobs.bind(4, limitResultRows);

        for (auto job : q_selectJobs)
        {
            // Check every 4 items
            if (jobs.size() % 4 == 0 && wasStopped())
                return;

            item.jobId = job.get<db_id>(0);
            jobs.append(item);
        }
        q_selectJobs.reset();
    }
}

void SearchTask::searchByNum(const QString &num, QList<SearchResultItem> &jobs)
{
    if (queryType != ByNum)
    {
        q_selectJobs.prepare("SELECT id, category, id LIKE ?1 as job_rank FROM jobs"
                             " WHERE id LIKE ?2"
                             " ORDER BY job_rank DESC, id ASC LIMIT ?3");
        queryType = ByNum;
    }

    q_selectJobs.bind(1, num + '%');
    q_selectJobs.bind(2, '%' + num + '%');
    q_selectJobs.bind(3, limitResultRows);

    for (auto job : q_selectJobs)
    {
        // Check every 4 items
        if (jobs.size() % 4 == 0 && wasStopped())
            return;

        SearchResultItem item;
        item.jobId    = job.get<db_id>(0);
        item.category = JobCategory(job.get<int>(1));
        jobs.append(item);
    }
    q_selectJobs.reset();
}

void SearchTask::setQuery(const QString &query)
{
    mQuery = query;
}

#endif // SEARCHBOX_MODE_ASYNC
