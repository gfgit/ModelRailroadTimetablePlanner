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

#ifndef SQLCONSOLE_H
#define SQLCONSOLE_H

#ifdef ENABLE_USER_QUERY

#include <QDialog>

class SQLViewer;
class QPlainTextEdit;
class QSpinBox;
class QTimer;

class SQLConsole : public QDialog
{
    Q_OBJECT
public:
    explicit SQLConsole(QWidget *parent = nullptr);
    ~SQLConsole();

    void setInterval(int secs);

public slots:
    bool executeQuery();

private slots:
    void onIntervalChangedUser();

    void timedExec();
private:
    QPlainTextEdit *edit;
    SQLViewer *viewer;
    QSpinBox *intervalSpin;
    QTimer *timer;
};

#endif // ENABLE_USER_QUERY

#endif // SQLCONSOLE_H
