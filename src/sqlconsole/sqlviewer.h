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
