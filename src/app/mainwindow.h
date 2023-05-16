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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "utils/types.h"

namespace Ui {
class MainWindow;
}

namespace sqlite3pp {
class database;
}

using namespace sqlite3pp;

class LineGraphWidget;
class JobPathEditor;
class QDockWidget;
class QLabel;
class QActionGroup;
class CustomCompletionLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void enableDBActions(bool enable);
    void setCurrentFile(const QString &fileName);
    void updateRecentFileActions();

    QString strippedName(const QString &fullFileName, bool *ok = nullptr);
    void loadFile(const QString &fileName);

    bool closeSession();
    
private slots:
    void onStationManager();

    void onNew();
    void onOpen();
    void onOpenRecent();
    void onCloseSession();

    void onProperties();
    void onMeetingInformation();

    void onRollingStockManager();

    void onShiftManager();

    void onJobsManager();

    void onAddJob();
    void onRemoveJob();

    void about();
    void onPrint();
    void onPrintPDF();
    void onExportSvg();

#ifdef ENABLE_USER_QUERY
    void onExecQuery();
#endif

    void onSaveCopyAs();
    void onSave();
    void onOpenSettings();

    void checkLineNumber();

protected:
    void timerEvent(QTimerEvent *e) override;

    void closeEvent(QCloseEvent *e) override;

    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onJobSelected(db_id jobId);

    void onSessionRSViewer();

    void onJobSearchItemSelected();
    void onJobSearchResultsReady();

private:
    void setup_actions();
    void showCloseWarning();
    void stopCloseTimer();

    enum class CentralWidgetMode
    {
        StartPageMode,
        NoLinesWarningMode,
        ViewSessionMode
    };

    void setCentralWidgetMode(CentralWidgetMode mode);

private:
    Ui::MainWindow *ui;

    JobPathEditor *jobEditor;

#ifdef ENABLE_BACKGROUND_MANAGER
    QDockWidget *resPanelDock;
#endif // ENABLE_BACKGROUND_MANAGER

    LineGraphWidget *view;
    QDockWidget *jobDock;

    CustomCompletionLineEdit *searchEdit;

    QLabel *welcomeLabel;

    QActionGroup *databaseActionGroup;

    enum { MaxRecentFiles = 5 };
    QAction* recentFileActs[MaxRecentFiles];

    CentralWidgetMode m_mode;

    int closeTimerId;
};

#endif // MAINWINDOW_H
