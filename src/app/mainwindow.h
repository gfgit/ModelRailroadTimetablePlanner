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

#ifdef ENABLE_RS_CHECKER
class RsErrorsWidget;
#endif

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

#ifdef ENABLE_RS_CHECKER
    RsErrorsWidget *rsErrorsWidget;
    QDockWidget *rsErrDock;
#endif

    LineGraphWidget *view;
    QDockWidget *jobDock;

    CustomCompletionLineEdit *searchEdit;

    QLabel *welcomeLabel;

    QActionGroup *databaseActionGroup;

    enum { MaxRecentFiles = 5 };
    QAction* recentFileActs[MaxRecentFiles];

    CentralWidgetMode m_mode;
};

#endif // MAINWINDOW_H
