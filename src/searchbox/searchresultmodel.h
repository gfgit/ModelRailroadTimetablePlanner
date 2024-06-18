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

#ifndef SEARCHRESULTMODEL_H
#define SEARCHRESULTMODEL_H

#include "utils/delegates/sql/isqlfkmatchmodel.h"

#include <QList>

#include <QFont>

#include "utils/types.h"

#include "searchresultitem.h"

#ifdef SEARCHBOX_MODE_ASYNC
class SearchTask;
#endif

namespace sqlite3pp {
class database;
}

class SearchResultModel : public ISqlFKMatchModel
{
    Q_OBJECT

public:
    enum Column
    {
        JobCategoryCol = 0,
        JobNumber,
        NCols
    };

    SearchResultModel(sqlite3pp::database &db, QObject *parent = nullptr);
    ~SearchResultModel();

#ifdef SEARCHBOX_MODE_ASYNC
    bool event(QEvent *ev) override;
#endif

    // Basic functionality:
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // ISqlFKMatchModel
    virtual void autoSuggest(const QString &text) override;
    virtual void clearCache() override;

    virtual db_id getIdAtRow(int row) const override;
    virtual QString getNameAtRow(int row) const override;

public slots:
#ifdef SEARCHBOX_MODE_ASYNC
    void abortSearch();
    void stopAllTasks();
#endif

private:
    sqlite3pp::database &mDb;

#ifdef SEARCHBOX_MODE_ASYNC
    QList<SearchTask *> tasks;
    SearchTask *reusableTask;
    int deleteReusableTaskTimerId;

    void disposeTask(SearchTask *task);
    SearchTask *createTask(const QString &text);
    void clearReusableTask();
#endif

    QList<SearchResultItem> m_data;

    QFont m_font;

    QStringList catNames, catNamesAbbr;
};

#endif // SEARCHRESULTMODEL_H
