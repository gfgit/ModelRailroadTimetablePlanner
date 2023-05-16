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

#ifndef PRINTWORKERHANDLER_H
#define PRINTWORKERHANDLER_H

#include <QObject>

#include "printdefs.h"
#include "printing/helper/model/printhelper.h"

class PrintWorker;
class IGraphSceneCollection;

namespace sqlite3pp {
class database;
}

class PrintWorkerHandler : public QObject
{
    Q_OBJECT
public:
    explicit PrintWorkerHandler(sqlite3pp::database &db, QObject *parent = nullptr);

    inline Print::PrintBasicOptions getOptions() const { return printOpt; }
    void setOptions(const Print::PrintBasicOptions& opt, QPrinter *printer);

    inline Print::PageLayoutOpt getScenePageLay() const { return scenePageLay; }
    inline void setScenePageLay(const Print::PageLayoutOpt& lay) { scenePageLay = lay; }

    bool event(QEvent *e) override;

    inline bool taskIsRunning() const { return printTask != nullptr; }
    inline bool waitingForTaskToStop() const { return isStoppingTask; }

signals:
    void progressMaxChanged(int max);
    void progressChanged(int val, const QString& msg);
    void progressFinished(bool success, const QString& msg);

public:
    void startPrintTask(QPrinter *printer, IGraphSceneCollection *collection);
    void abortPrintTask();
    void stopTaskGracefully();

private:
    sqlite3pp::database &mDb;

    PrintWorker *printTask;

    Print::PrintBasicOptions printOpt;
    Print::PageLayoutOpt scenePageLay;

    bool isStoppingTask;
};

#endif // PRINTWORKERHANDLER_H
