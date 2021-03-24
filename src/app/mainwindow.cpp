#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "app/session.h"

#include "viewmanager/viewmanager.h"

#include <QStandardPaths>
#include <QFileDialog>
#include "utils/file_format_names.h"

#include "jobs/jobeditor/jobpatheditor.h"
#include <QDockWidget>

#include <QPointer>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>

#include <QCloseEvent>

#include "settings/settingsdialog.h"

#include "graph/graphmanager.h"
#include "graph/graphicsview.h"
#include <QGraphicsItem>

#include "db_metadata/meetinginformationdialog.h"

#include "lines/linestorage.h"
#include "lines/linesmatchmodel.h"

#include "printing/printwizard.h"

#ifdef ENABLE_USER_QUERY
#include "sqlconsole/sqlconsole.h"
#endif

#include <QActionGroup>

#include "utils/sqldelegate/customcompletionlineedit.h"
#include "searchbox/searchresultmodel.h"

#ifdef ENABLE_BACKGROUND_MANAGER
#include "backgroundmanager/backgroundmanager.h"
#endif

#ifdef ENABLE_RS_CHECKER
#include "rollingstock/rs_checker/rserrorswidget.h"
#endif

#include "propertiesdialog.h"
#include "info.h"

#include <QThreadPool>

#include <QTimer> //HACK: TODO remove

#include "app/scopedebug.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    jobEditor(nullptr),
#ifdef ENABLE_RS_CHECKER
    rsErrorsWidget(nullptr),
    rsErrDock(nullptr),
#endif
    view(nullptr),
    jobDock(nullptr),
    searchEdit(nullptr),
    lineComboSearch(nullptr),
    welcomeLabel(nullptr),
    recentFileActs{nullptr},
    m_mode(CentralWidgetMode::StartPageMode)
{
    ui->setupUi(this);
    ui->actionAbout->setText(tr("About %1").arg(qApp->applicationDisplayName()));

    auto viewMgr = Session->getViewManager();
    viewMgr->m_mainWidget = this;

    auto graphMgr = viewMgr->getGraphMgr();
    connect(graphMgr, &GraphManager::jobSelected, this, &MainWindow::onJobSelected);

    view = graphMgr->getView();
    view->setObjectName("GraphicsView");
    view->setParent(this);

    auto linesMatchModel = new LinesMatchModel(Session->m_Db, true, this);
    linesMatchModel->setHasEmptyRow(false); //Do not allow empty view (no line selected)
    lineComboSearch = new CustomCompletionLineEdit(linesMatchModel, this);
    lineComboSearch->setPlaceholderText(tr("Choose line"));
    lineComboSearch->setToolTip(tr("Choose a railway line"));
    lineComboSearch->setMinimumWidth(200);
    lineComboSearch->setMinimumHeight(25);
    lineComboSearch->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QAction *sep = ui->mainToolBar->insertSeparator(ui->actionPrev_Job_Segment);
    ui->mainToolBar->insertWidget(sep, lineComboSearch);

    connect(lineComboSearch, &CustomCompletionLineEdit::dataIdChanged, graphMgr, &GraphManager::setCurrentLine);
    connect(graphMgr, &GraphManager::currentLineChanged, lineComboSearch, &CustomCompletionLineEdit::setData_slot);

    auto lineStorage = Session->mLineStorage;
    connect(lineStorage, &LineStorage::lineAdded, this, &MainWindow::checkLineNumber);
    connect(lineStorage, &LineStorage::lineRemoved, this, &MainWindow::checkLineNumber);

    //Welcome label
    welcomeLabel = new QLabel(this);
    welcomeLabel->setTextFormat(Qt::RichText);
    welcomeLabel->setAlignment(Qt::AlignCenter);
    welcomeLabel->setFont(QFont("Arial", 15));
    welcomeLabel->setObjectName("WelcomeLabel");

    //JobPathEditor dock
    jobEditor = new JobPathEditor(this);
    viewMgr->jobEditor = jobEditor;
    jobDock = new QDockWidget("JobEditor", this);
    jobDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    jobDock->setWidget(jobEditor);
    jobDock->installEventFilter(this); //NOTE: see MainWindow::eventFilter() below

    addDockWidget(Qt::RightDockWidgetArea, jobDock);
    ui->menuView->addAction(jobDock->toggleViewAction());
    connect(jobDock->toggleViewAction(), &QAction::triggered, jobEditor, &JobPathEditor::show);

#ifdef ENABLE_RS_CHECKER
    //RS Errors dock
    rsErrorsWidget = new RsErrorsWidget(this);
    rsErrDock = new QDockWidget(rsErrorsWidget->windowTitle(), this);
    rsErrDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    rsErrDock->setWidget(rsErrorsWidget);
    rsErrDock->installEventFilter(this); //NOTE: see eventFilter() below

    addDockWidget(Qt::BottomDockWidgetArea, rsErrDock);
    ui->menuView->addAction(rsErrDock->toggleViewAction());
    ui->mainToolBar->addAction(rsErrDock->toggleViewAction());
#endif

    //Allow JobPathEditor to use all vertical space when RsErrorWidget dock is at bottom
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    //Search Box
    SearchResultModel *searchModel = new SearchResultModel(Session->m_Db, this);
    searchEdit = new CustomCompletionLineEdit(searchModel, this);
    searchEdit->setMinimumWidth(300);
    searchEdit->setMinimumHeight(25);
    searchEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    searchEdit->setPlaceholderText(tr("Find"));
    searchEdit->setClearButtonEnabled(true);
    connect(searchEdit, &CustomCompletionLineEdit::dataIdChanged, this, &MainWindow::onJobSearchItemSelected);
    connect(searchModel, &SearchResultModel::resultsReady, this, &MainWindow::onJobSearchResultsReady);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->mainToolBar->addWidget(spacer);
    ui->mainToolBar->addWidget(searchEdit);

    setup_actions();
    setCentralWidgetMode(CentralWidgetMode::StartPageMode);

    QMenu *recentFilesMenu = new QMenu(this);
    for(int i = 0; i < MaxRecentFiles; i++)
    {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], &QAction::triggered,
                this, &MainWindow::onOpenRecent);

        recentFilesMenu->addAction(recentFileActs[i]);
    }

    updateRecentFileActions();

    ui->actionOpen_Recent->setMenu(recentFilesMenu);
}

MainWindow::~MainWindow()
{
    Session->getViewManager()->m_mainWidget = nullptr;
    delete ui;
}

void MainWindow::setup_actions()
{
    databaseActionGroup = new QActionGroup(this);

    databaseActionGroup->addAction(ui->actionAddJob);
    databaseActionGroup->addAction(ui->actionRemoveJob);

    databaseActionGroup->addAction(ui->actionStations);
    databaseActionGroup->addAction(ui->actionRollingstockManager);
    databaseActionGroup->addAction(ui->actionJob_Shifts);
    databaseActionGroup->addAction(ui->action_JobsMgr);
    databaseActionGroup->addAction(ui->actionRS_Session_Viewer);
    databaseActionGroup->addAction(ui->actionMeeting_Information);

    databaseActionGroup->addAction(ui->actionQuery);

    databaseActionGroup->addAction(ui->actionClose);
    databaseActionGroup->addAction(ui->actionPrint);

    databaseActionGroup->addAction(ui->actionSave);
    databaseActionGroup->addAction(ui->actionSaveCopy_As);

    databaseActionGroup->addAction(ui->actionExport_PDF);
    databaseActionGroup->addAction(ui->actionExport_Svg);

    databaseActionGroup->addAction(ui->actionPrev_Job_Segment);
    databaseActionGroup->addAction(ui->actionNext_Job_Segment);

    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpen);
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::onNew);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::onCloseSession);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::onSave);
    connect(ui->actionSaveCopy_As, &QAction::triggered, this, &MainWindow::onSaveCopyAs);

    connect(ui->actionPrint, &QAction::triggered, this, &MainWindow::onPrint);
    connect(ui->actionExport_PDF, &QAction::triggered, this, &MainWindow::onPrintPDF);
    connect(ui->actionExport_Svg, &QAction::triggered, this, &MainWindow::onExportSvg);
    connect(ui->actionProperties, &QAction::triggered, this, &MainWindow::onProperties);

    connect(ui->actionStations, &QAction::triggered, this, &MainWindow::onStationManager);
    connect(ui->actionRollingstockManager, &QAction::triggered, this, &MainWindow::onRollingStockManager);
    connect(ui->actionJob_Shifts, &QAction::triggered, this, &MainWindow::onShiftManager);
    connect(ui->action_JobsMgr, &QAction::triggered, this, &MainWindow::onJobsManager);
    connect(ui->actionRS_Session_Viewer, &QAction::triggered, this, &MainWindow::onSessionRSViewer);
    connect(ui->actionMeeting_Information, &QAction::triggered, this, &MainWindow::onMeetingInformation);

    connect(ui->actionAddJob, &QAction::triggered, this, &MainWindow::onAddJob);
    connect(ui->actionRemoveJob, &QAction::triggered, this, &MainWindow::onRemoveJob);

    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);

#ifdef ENABLE_USER_QUERY
    connect(ui->actionQuery, &QAction::triggered, this, &MainWindow::onExecQuery);
#else
    ui->actionQuery->setVisible(false);
    ui->actionQuery->setEnabled(false);
#endif

    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onOpenSettings);

    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);

    connect(ui->actionNext_Job_Segment, &QAction::triggered, this, []()
            {
                Session->getViewManager()->requestJobShowPrevNextSegment(false);
            });
    connect(ui->actionPrev_Job_Segment, &QAction::triggered, this, []()
            {
                Session->getViewManager()->requestJobShowPrevNextSegment(true);
            });
}

void MainWindow::about()
{
    QMessageBox::information(this,
                             tr("About"),
                             tr("%1 application makes easier to deal with timetables and trains\n"
                                "Beta version: %2\n"
                                "Built: %3")
                                 .arg(qApp->applicationDisplayName())
                                 .arg(qApp->applicationVersion())
                                 .arg(QDate::fromString(AppBuildDate, QLatin1String("MMM dd yyyy")).toString("dd/MM/yyyy")));
}

void MainWindow::onOpen()
{
    DEBUG_ENTRY;

#ifdef SEARCHBOX_MODE_ASYNC
    Session->getBackgroundManager()->abortTrivialTasks();
#endif

#ifdef ENABLE_BACKGROUND_MANAGER
    if(Session->getBackgroundManager()->isRunning())
    {
        int ret = QMessageBox::warning(this,
                                       tr("Backgroung Task"),
                                       tr("Background task for checking rollingstock errors is still running.\n"
                                          "Do you want to cancel it?"),
                                       QMessageBox::Yes, QMessageBox::No,
                                       QMessageBox::Yes);
        if(ret == QMessageBox::Yes)
            Session->getBackgroundManager()->abortAllTasks();
        else
            return;
    }
#endif

    QFileDialog dlg(this, tr("Open Session"));
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::tttFormat);
    filters << FileFormats::tr(FileFormats::sqliteFormat);
    filters << FileFormats::tr(FileFormats::allFiles);
    dlg.setNameFilters(filters);

    if(dlg.exec() != QDialog::Accepted)
        return;

    QString fileName = dlg.selectedUrls().value(0).toLocalFile();

    if(fileName.isEmpty())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if(!QThreadPool::globalInstance()->waitForDone(2000))
    {
        QMessageBox::warning(this,
                             tr("Background Tasks"),
                             tr("Some background tasks are still running.\n"
                                "The file was not opened. Try again."));
        QApplication::restoreOverrideCursor();
        return;
    }

    QApplication::restoreOverrideCursor();

    loadFile(fileName);
}

void MainWindow::loadFile(const QString& fileName)
{
    DEBUG_ENTRY;
    if(fileName.isEmpty())
        return;

    qDebug() << "Loading:" << fileName;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    DB_Error err = Session->openDB(fileName, false);

    QApplication::restoreOverrideCursor();

    if(err == DB_Error::FormatTooOld)
    {
        int but = QMessageBox::warning(this, tr("Version is old"),
                                       tr("This file was created by an older version of Train Timetable.\n"
                                          "Opening it without conversion might not work and even crash the application.\n"
                                          "Do you want to open it anyway?"),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if(but == QMessageBox::Yes)
            err = Session->openDB(fileName, true);
    }
    else if(err == DB_Error::FormatTooNew)
    {
        if(err == DB_Error::FormatTooOld)
        {
            int but = QMessageBox::warning(this, tr("Version is too new"),
                                           tr("This file was created by a newer version of Train Timetable.\n"
                                              "You should update the application first. Opening this file might not work or even crash.\n"
                                              "Do you want to open it anyway?"),
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if(but == QMessageBox::Yes)
                err = Session->openDB(fileName, true);
        }
    }

    if(err == DB_Error::DbBusyWhenClosing)
        showCloseWarning();

    if(err != DB_Error::NoError)
        return;

    setCurrentFile(fileName);

    //Fake we are coming from Start Page
    //Otherwise we cannot show the first line
    m_mode = CentralWidgetMode::StartPageMode;
    checkLineNumber();


    if(!Session->checkImportRSTablesEmpty())
    {
        //Probably the application crashed before finishing RS importation
        //Give user choice to resume it or discard

        QPointer<QMessageBox> msgBox = new QMessageBox(
            QMessageBox::Warning,
            tr("RS Import"),
            tr("There is some rollingstock import data left in this file. "
               "Probably the application has crashed!<br>"
               "Before deleting it would you like to resume importation?<br>"
               "<i>(Sorry for the crash, would you like to contact me and share information about it?)</i>"),
            QMessageBox::NoButton, this);
        auto resumeBut = msgBox->addButton(tr("Resume importation"), QMessageBox::YesRole);
        msgBox->addButton(tr("Just delete it"), QMessageBox::NoRole);
        msgBox->setDefaultButton(resumeBut);
        msgBox->setTextFormat(Qt::RichText);

        msgBox->exec();

        if(msgBox)
        {
            if(msgBox->clickedButton() == resumeBut)
            {
                Session->getViewManager()->resumeRSImportation();
            }else{
                Session->clearImportRSTables();
            }
        }

        delete msgBox;
    }
}

void MainWindow::setCurrentFile(const QString& fileName)
{
    DEBUG_ENTRY;

    if(fileName.isEmpty())
    {
        setWindowFilePath(QString()); //Reset title bar
        return;
    }

    //Qt automatically takes care of showing stripped filename in window title
    setWindowFilePath(fileName);

    QStringList files = AppSettings.getRecentFiles();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    AppSettings.setRecentFiles(files);

    updateRecentFileActions();
}

QString MainWindow::strippedName(const QString &fullFileName, bool *ok)
{
    QFileInfo fi(fullFileName);
    if(ok) *ok = fi.exists();
    return fi.fileName();
}

void MainWindow::updateRecentFileActions()
{
    DEBUG_ENTRY;
    QStringList files = AppSettings.getRecentFiles();

    int numRecentFiles = qMin(files.size(), int(MaxRecentFiles));

    for (int i = 0; i < numRecentFiles; i++)
    {
        bool ok = true;
        QString name = strippedName(files[i], &ok);
        if(name.isEmpty() || !ok)
        {
            files.removeAt(i);
            i--;
            numRecentFiles = qMin(files.size(), int(MaxRecentFiles));
        }
        else
        {
            QString text = tr("&%1 %2").arg(i + 1).arg(name);
            recentFileActs[i]->setText(text);
            recentFileActs[i]->setData(files[i]);
            recentFileActs[i]->setToolTip(files[i]);
            recentFileActs[i]->setVisible(true);
        }
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActs[j]->setVisible(false);

    AppSettings.setRecentFiles(files);
}

void MainWindow::onOpenRecent()
{
    DEBUG_ENTRY;
    QAction *act = qobject_cast<QAction*>(sender());
    if(!act)
        return;

    loadFile(act->data().toString());
}

void MainWindow::onNew()
{
    DEBUG_ENTRY;

#ifdef SEARCHBOX_MODE_ASYNC
    Session->getBackgroundManager()->abortTrivialTasks();
#endif

#ifdef ENABLE_RS_CHECKER
    if(Session->getBackgroundManager()->isRunning())
    {
        int ret = QMessageBox::warning(this,
                                       tr("Backgroung Task"),
                                       tr("Background task for checking rollingstock errors is still running.\n"
                                          "Do you want to cancel it?"),
                                       QMessageBox::Yes, QMessageBox::No,
                                       QMessageBox::Yes);
        if(ret == QMessageBox::Yes)
            Session->getBackgroundManager()->abortAllTasks();
        else
            return;
    }
#endif

    QFileDialog dlg(this, tr("Create new Session"));
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::tttFormat);
    filters << FileFormats::tr(FileFormats::sqliteFormat);
    filters << FileFormats::tr(FileFormats::allFiles);
    dlg.setNameFilters(filters);

    if(dlg.exec() != QDialog::Accepted)
        return;

    QString fileName = dlg.selectedUrls().value(0).toLocalFile();

    if(fileName.isEmpty())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if(!QThreadPool::globalInstance()->waitForDone(2000))
    {
        QMessageBox::warning(this,
                             tr("Background Tasks"),
                             tr("Some background tasks are still running.\n"
                                "The new file was not created. Try again."));
        QApplication::restoreOverrideCursor();
        return;
    }

    QFile f(fileName);
    if(f.exists())
        f.remove();

    DB_Error err = Session->createNewDB(fileName);

    QApplication::restoreOverrideCursor();

    if(err == DB_Error::DbBusyWhenClosing)
        showCloseWarning();

    if(err != DB_Error::NoError)
        return;

    setCurrentFile(fileName);
    checkLineNumber();
}

void MainWindow::onSave()
{
    if(!Session->getViewManager()->closeEditors())
        return;

    Session->releaseAllSavepoints();
}

void MainWindow::onSaveCopyAs()
{
    DEBUG_ENTRY;

    if(!Session->getViewManager()->closeEditors())
        return;

    QFileDialog dlg(this, tr("Save Session Copy"));
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

    QStringList filters;
    filters << FileFormats::tr(FileFormats::tttFormat);
    filters << FileFormats::tr(FileFormats::sqliteFormat);
    filters << FileFormats::tr(FileFormats::allFiles);
    dlg.setNameFilters(filters);

    if(dlg.exec() != QDialog::Accepted)
        return;

    QString fileName = dlg.selectedUrls().value(0).toLocalFile();

    if(fileName.isEmpty())
        return;

    QFile f(fileName);
    if(f.exists())
        f.remove();

    database backupDB(fileName.toUtf8(), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    int rc = Session->m_Db.backup(backupDB, [](int pageCount, int remaining, int res)
                                  {
                                      Q_UNUSED(res)
                                      qDebug() << pageCount << "/" << remaining;
                                  });

    if(rc != SQLITE_OK && rc != SQLITE_DONE)
    {
        QString errMsg = Session->m_Db.error_msg();
        qDebug() << Session->m_Db.error_code() << errMsg;
        QMessageBox::warning(this, tr("Error saving copy"), errMsg);
    }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if(closeSession())
        e->accept();
    else
        e->ignore();
}

void MainWindow::showCloseWarning()
{
    QMessageBox::warning(this,
                         tr("Error while Closing"),
                         tr("There was an error while closing the database.\n"
                            "Make sure there aren't any background tasks running and try again."));
}

void MainWindow::setCentralWidgetMode(MainWindow::CentralWidgetMode mode)
{
    switch (mode)
    {
    case CentralWidgetMode::StartPageMode:
    {
        jobDock->hide();
#ifdef ENABLE_RS_CHECKER
        rsErrDock->hide();
#endif
        welcomeLabel->setText(tr("<p>Open a file: <b>File</b> > <b>Open</b></p>"
                                 "<p>Create new project: <b>File</b> > <b>New</b></p>"));
        statusBar()->showMessage(tr("Open file or create a new one"));

        break;
    }
    case CentralWidgetMode::NoLinesWarningMode:
    {
        jobDock->show();
#ifdef ENABLE_RS_CHECKER
        rsErrDock->hide();
#endif
        welcomeLabel->setText(
            tr("<p><b>There are no lines in this session</b></p>"
               "<p>"
               "<table align=\"center\">"
               "<tr>"
               "<td>Start by creating the railway layout for this session:</td>"
               "</tr>"
               "<tr>"
               "<td>"
               "<table>"
               "<tr>"
               "<td>1.</td>"
               "<td>Create stations (<b>Edit</b> > <b>Stations</b>)</td>"
               "</tr>"
               "<tr>"
               "<td>2.</td>"
               "<td>Create railway lines (<b>Edit</b> > <b>Stations</b> > <b>Lines Tab</b>)</td>"
               "</tr>"
               "<tr>"
               "<td>3.</td>"
               "<td>Add stations to railway lines</td>"
               "</tr>"
               "<tr>"
               "<td></td>"
               "<td>(<b>Edit</b> > <b>Stations</b> > <b>Lines Tab</b> > <b>Edit Line</b>)</td>"
               "</tr>"
               "</table>"
               "</td>"
               "</tr>"
               "</table>"
               "</p>"));
        break;
    }
    case CentralWidgetMode::ViewSessionMode:
    {
        jobDock->show();
#ifdef ENABLE_RS_CHECKER
        rsErrDock->show();
#endif
        welcomeLabel->setText(QString());
        break;
    }
    }

    if(mode == CentralWidgetMode::ViewSessionMode)
    {
        if(centralWidget() != view)
        {
            takeCentralWidget(); //Remove ownership from welcomeLabel
            setCentralWidget(view);
            view->show();
            welcomeLabel->hide();
        }
    }
    else
    {
        if(centralWidget() != welcomeLabel)
        {
            takeCentralWidget(); //Remove ownership from QGraphicsView
            setCentralWidget(welcomeLabel);
            view->hide();
            welcomeLabel->show();
        }
    }

    enableDBActions(mode != CentralWidgetMode::StartPageMode);

    m_mode = mode;
}

void MainWindow::onCloseSession()
{
    closeSession();
}

void MainWindow::onProperties()
{
    PropertiesDialog dlg(this);
    dlg.exec();
}

void MainWindow::onMeetingInformation()
{
    MeetingInformationDialog dlg(this);
    if(dlg.exec() == QDialog::Accepted)
        dlg.saveData();
}

bool MainWindow::closeSession()
{
    DB_Error err = Session->closeDB();

    if(err == DB_Error::DbBusyWhenClosing)
        showCloseWarning();

    if(err != DB_Error::NoError && err != DB_Error::DbNotOpen)
        return false;

    setCentralWidgetMode(CentralWidgetMode::StartPageMode);

    //Reset filePath to refresh title
    setCurrentFile(QString());

    lineComboSearch->setData(0);

    return true;
}

void MainWindow::enableDBActions(bool enable)
{
    databaseActionGroup->setEnabled(enable);
    searchEdit->setEnabled(enable);
    lineComboSearch->setEnabled(enable);
    if(!enable)
        jobEditor->setEnabled(false);

#ifdef ENABLE_RS_CHECKER
    rsErrorsWidget->setEnabled(enable);
#endif
}

void MainWindow::onStationManager()
{
    Session->getViewManager()->showStationsManager();
}

void MainWindow::onRollingStockManager()
{
    Session->getViewManager()->showRSManager();
}

void MainWindow::onShiftManager()
{
    Session->getViewManager()->showShiftManager();
}

void MainWindow::onJobsManager()
{
    Session->getViewManager()->showJobsManager();
}

void MainWindow::onAddJob()
{
    Session->getViewManager()->requestJobCreation();
}

void MainWindow::onRemoveJob()
{
    DEBUG_ENTRY;
    Session->getViewManager()->removeSelectedJob();
}

void MainWindow::onPrint()
{
    PrintWizard wizard(this);
    wizard.setOutputType(Print::Native);
    wizard.exec();
}

void MainWindow::onPrintPDF()
{
    PrintWizard wizard(this);
    wizard.setOutputType(Print::Pdf);
    wizard.exec();
}

void MainWindow::onExportSvg()
{
    PrintWizard wizard(this);
    wizard.setOutputType(Print::Svg);
    wizard.exec();
}

#ifdef ENABLE_USER_QUERY
void MainWindow::onExecQuery()
{
    DEBUG_ENTRY;
    SQLConsole *console = new SQLConsole(this);
    console->setAttribute(Qt::WA_DeleteOnClose);
    console->show();
}
#endif

void MainWindow::onOpenSettings()
{
    DEBUG_ENTRY;
    SettingsDialog dlg(this);
    dlg.loadSettings();
    dlg.exec();
}

void MainWindow::checkLineNumber()
{
    auto graphMgr = Session->getViewManager()->getGraphMgr();
    db_id firstLineId = graphMgr->getFirstLineId();

    if(firstLineId && m_mode != CentralWidgetMode::ViewSessionMode)
    {
        //First line was added or newly opened file -> Session has at least one line
        ui->actionAddJob->setEnabled(true);
        ui->actionAddJob->setToolTip(tr("Add train job"));
        ui->actionRemoveJob->setEnabled(true);
        setCentralWidgetMode(CentralWidgetMode::ViewSessionMode);

        //Show first line (Alphabetically)
        graphMgr->setCurrentLine(firstLineId);
    }
    else if(firstLineId == 0 && m_mode != CentralWidgetMode::NoLinesWarningMode)
    {
        //Last line removed -> Session has no line
        //If there aren't lines prevent from creating jobs
        ui->actionAddJob->setEnabled(false);
        ui->actionAddJob->setToolTip(tr("You must create at least one line before adding job to this session"));
        ui->actionRemoveJob->setEnabled(false);
        setCentralWidgetMode(CentralWidgetMode::NoLinesWarningMode);
    }
}

void MainWindow::onJobSelected(db_id jobId)
{
    const bool selected = jobId != 0;
    ui->actionPrev_Job_Segment->setEnabled(selected);
    ui->actionNext_Job_Segment->setEnabled(selected);
}

//QT-BUG: If user closes a floating dock widget, when shown again it cannot dock anymore
//HACK: intercept dock close event and manually re-dock and hide so next time is shown it's docked
//NOTE: calling directly 'QDockWidget::setFloating(false)' from inside 'eventFinter()' causes CRASH
//      so queue it. Cannot use 'QMetaObject::invokeMethod()' because it's not a slot.
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == jobDock && event->type() == QEvent::Close)
    {
        if(jobDock->isFloating())
        {
            QTimer::singleShot(0, jobDock, [this]()
                               {
                                   jobDock->setFloating(false);
                               });
        }
    }
#ifdef ENABLE_RS_CHECKER
    else if(watched == rsErrDock && event->type() == QEvent::Close)
    {
        if(rsErrDock->isFloating())
        {
            QTimer::singleShot(0, rsErrDock, [this]()
                               {
                                   rsErrDock->setFloating(false);
                               });
        }
    }
#endif

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onSessionRSViewer()
{
    Session->getViewManager()->showSessionStartEndRSViewer();
}

void MainWindow::onJobSearchItemSelected(db_id jobId)
{
    searchEdit->clear(); //Clear text
    Session->getViewManager()->requestJobSelection(jobId, true, true);
}

void MainWindow::onJobSearchResultsReady()
{
    searchEdit->resizeColumnToContents();
    searchEdit->selectFirstIndexOrNone(true);
}
