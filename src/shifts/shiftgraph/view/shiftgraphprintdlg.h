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

#ifndef SHIFTGRAPHPRINTDLG_H
#define SHIFTGRAPHPRINTDLG_H

#include <QDialog>

#include "printing/helper/model/printdefs.h"

class PrinterOptionsWidget;
class QDialogButtonBox;

class QGroupBox;
class QLabel;
class QProgressBar;

class QPrinter;
class ShiftGraphSceneCollection;

class PrintWorkerHandler;

namespace sqlite3pp {
class database;
}

namespace Print {
struct PageLayoutOpt;
}

class ShiftGraphPrintDlg : public QDialog
{
    Q_OBJECT
public:
    explicit ShiftGraphPrintDlg(sqlite3pp::database &db, QWidget *parent = nullptr);
    ~ShiftGraphPrintDlg();

    void done(int res) override;

    void setOutputType(Print::OutputType type);

private slots:
    void updatePrintButton();
    void progressMaxChanged(int max);
    void progressChanged(int val, const QString& msg);
    void handleProgressFinished(bool success, const QString& errMsg);

private:
    sqlite3pp::database &mDb;

    PrinterOptionsWidget *optionsWidget;
    QDialogButtonBox *buttonBox;

    QGroupBox *progressBox;
    QLabel *progressLabel;
    QProgressBar *progressBar;

    ShiftGraphSceneCollection *m_collection;
    QPrinter *m_printer;

    PrintWorkerHandler *printTaskHandler;
};

#endif // SHIFTGRAPHPRINTDLG_H
