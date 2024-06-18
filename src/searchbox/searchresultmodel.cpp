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

#include "searchresultmodel.h"

#include "searchresultitem.h"

#include "app/session.h"
#include "utils/jobcategorystrings.h"

#ifdef SEARCHBOX_MODE_ASYNC

#    ifndef ENABLE_BACKGROUND_MANAGER
#        error "Cannot use SEARCHBOX_MODE_ASYNC without ENABLE_BACKGROUND_MANAGER"
#    endif

#    include "searchtask.h"
#    include "searchresultevent.h"
#    include <QThreadPool>

#    include "app/session.h"
#    include "backgroundmanager/backgroundmanager.h"
#endif

SearchResultModel::SearchResultModel(sqlite3pp::database &db, QObject *parent) :
    ISqlFKMatchModel(parent),
    mDb(db),
    reusableTask(nullptr),
    deleteReusableTaskTimerId(0)
{
    m_font.setPointSize(13);
    setHasEmptyRow(false);
    connect(Session->getBackgroundManager(), &BackgroundManager::abortTrivialTasks, this,
            &SearchResultModel::stopAllTasks);
}

SearchResultModel::~SearchResultModel()
{
#ifdef SEARCHBOX_MODE_ASYNC
    stopAllTasks();
#endif
}

int SearchResultModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : NCols;
}

QVariant SearchResultModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= size)
        return QVariant();

    const SearchResultItem &item = m_data.at(idx.row());

    switch (role)
    {
    case Qt::DisplayRole:
    {
        switch (idx.column())
        {
        case JobCategoryCol:
        {
            if (isEllipsesRow(idx.row()))
            {
                break;
            }
            return JobCategoryName::shortName(item.category);
        }
        case JobNumber:
        {
            if (isEllipsesRow(idx.row()))
            {
                return ellipsesString;
            }
            return item.jobId;
        }
        }
        break;
    }
    case Qt::ForegroundRole:
    {
        if (isEllipsesRow(idx.row()))
        {
            break;
        }
        switch (idx.column())
        {
        case JobCategoryCol:
        {
            return Session->colorForCat(item.category);
        }
        case JobNumber:
            return QColor(Qt::black);
        }
        break;
    }
    case Qt::FontRole:
    {
        switch (idx.column())
        {
        case JobCategoryCol:
        {
            QFont f = m_font;
            f.setWeight(QFont::Bold);
            return f;
        }
        case JobNumber:
            return m_font;
        }
    }
    }

    return QVariant();
}

void SearchResultModel::autoSuggest(const QString &text)
{
#ifdef SEARCHBOX_MODE_ASYNC
    abortSearch(); // Stop previous search

    SearchTask *task = createTask(text);
    tasks.append(task);

    QThreadPool::globalInstance()->start(task);
#else

    searchFor(editor->text()); // TODO
#endif
}

void SearchResultModel::clearCache()
{
    beginResetModel();
    m_data.clear();
    m_data.squeeze();
    size = 0;
    endResetModel();
}

db_id SearchResultModel::getIdAtRow(int row) const
{
    if (row >= m_data.size())
        return 0;
    return m_data.at(row).jobId;
}

QString SearchResultModel::getNameAtRow(int row) const
{
    if (row >= m_data.size())
        return QString();
    const SearchResultItem &item = m_data.at(row);
    return JobCategoryName::jobName(item.jobId, item.category);
}

#ifdef SEARCHBOX_MODE_ASYNC
void SearchResultModel::abortSearch()
{
    if (!tasks.isEmpty())
        tasks.last()->stop();
}

void SearchResultModel::stopAllTasks()
{
    for (SearchTask *task : std::as_const(tasks))
    {
        task->stop();
        task->cleanup();
    }
    tasks.clear();
    clearReusableTask();
}

void SearchResultModel::disposeTask(SearchTask *task)
{
    if (reusableTask)
    {
        delete task; // We already have a reusable task
    }
    else
    {
        reusableTask              = task;
        deleteReusableTaskTimerId = startTimer(3000, Qt::VeryCoarseTimer); // 3 sec, then delete
    }
}

SearchTask *SearchResultModel::createTask(const QString &text)
{
    if (catNames.isEmpty())
    {
        // Load categories names
        catNames.reserve(int(JobCategory::NCategories));
        for (int cat = 0; cat < int(JobCategory::NCategories); cat++)
        {
            catNames.append(JobCategoryName::tr(JobCategoryFullNameTable[cat]));
        }

        catNamesAbbr.reserve(int(JobCategory::NCategories));
        for (int cat = 0; cat < int(JobCategory::NCategories); cat++)
        {
            catNamesAbbr.append(JobCategoryName::tr(JobCategoryAbbrNameTable[cat]));
        }
    }

    SearchTask *task;
    if (reusableTask)
    {
        killTimer(deleteReusableTaskTimerId);
        deleteReusableTaskTimerId = 0;
        task                      = reusableTask;
        reusableTask              = nullptr;

        // NOTE: SearchTask gets disposed after finishing running so IQuittableTask has set
        // mReceiver to nullptr
        task->setReceiver(this);
    }
    else
    {
        task = new SearchTask(this, MaxMatchItems + 1, mDb, catNames, catNamesAbbr);
    }
    task->setQuery(text);
    return task;
}

void SearchResultModel::clearReusableTask()
{
    if (deleteReusableTaskTimerId)
    {
        killTimer(deleteReusableTaskTimerId);
        deleteReusableTaskTimerId = 0;
    }
    if (reusableTask)
    {
        delete reusableTask;
        reusableTask = nullptr;
    }
    if (tasks.isEmpty())
    {
        catNames.clear();
        catNamesAbbr.clear();
    }
}

bool SearchResultModel::event(QEvent *ev)
{
    if (ev->type() == QEvent::Timer
        && static_cast<QTimerEvent *>(ev)->timerId() == deleteReusableTaskTimerId)
    {
        clearReusableTask();
        return true;
    }
    if (ev->type() == SearchResultEvent::_Type)
    {
        SearchResultEvent *e = static_cast<SearchResultEvent *>(ev);
        e->setAccepted(true);
        int idx = tasks.indexOf(e->task);
        if (idx != -1)
        {
            bool wasLast = idx == tasks.size() - 1;
            tasks.removeAt(idx);
            disposeTask(e->task);

            if (wasLast)
            {
                beginResetModel();
                m_data = e->results;

                if (m_data.size() > MaxMatchItems)
                    size = MaxMatchItems + 1; // There would be still rows, show Ellipses
                else
                    size = m_data.size();

                endResetModel();
                emit resultsReady(true);
            }
        }

        return true;
    }

    return QObject::event(ev);
}
#endif
