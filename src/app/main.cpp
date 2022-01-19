#include "mainwindow.h"
#include <QApplication>
#include "app/session.h"

#include "utils/localization/languageutils.h"

#include <QTextStream>
#include <QFile>
#include <QDir>

#include <QDateTime>

#include "info.h"

#include <QThreadPool>

#include <QMutex>

#include <QDebug>

Q_GLOBAL_STATIC(QFile, gLogFile)
static QtMessageHandler defaultHandler;
static QMutex logMutex;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker lock(&logMutex);

    QString str;
    //const QString fmt = QStringLiteral("%1: %2 (%3:%4, %5)\n");
    static const QString fmt = QStringLiteral("%1: %2\n");
    switch (type) {
    case QtDebugMsg:
        str = fmt.arg(QStringLiteral("Debug"), msg); //.arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtInfoMsg:
        str = fmt.arg(QStringLiteral("Info"), msg); //.arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtWarningMsg:
        str = fmt.arg(QStringLiteral("Warning"), msg); //.arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtCriticalMsg:
        str = fmt.arg(QStringLiteral("Critical"), msg); //.arg(context.file).arg(context.line).arg(context.function);
        break;
    case QtFatalMsg:
        str = fmt.arg(QStringLiteral("Fatal"), msg); //.arg(context.file).arg(context.line).arg(context.function);
    }

    QTextStream s(gLogFile());
    s << str;

    defaultHandler(type, context, msg);
}

void setupLogger()
{
    //const QString path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString path = MeetingSession::appDataPath;
    if(qApp->arguments().contains("--test"))
        path = qApp->applicationDirPath(); //If testing use exe folder instead of AppData: see MeetingSession

    QFile *logFile = gLogFile();

    logFile->setFileName(path + QStringLiteral("/logs/mrtp_log.log"));
    logFile->open(QFile::WriteOnly | QFile::Append | QFile::Text);
    if(!logFile->isOpen()) //FIXME: if logFile gets too big, ask user to truncate it
    {
        QDir dir(path);
        dir.mkdir("logs");
        logFile->open(QFile::WriteOnly | QFile::Append | QFile::Text);
    }
    if(logFile->isOpen())
    {
        defaultHandler = qInstallMessageHandler(myMessageOutput);
    }
    else {
        qDebug() << "Cannot open Log file:" << logFile->fileName() << "Error:" << logFile->errorString();
    }
}

int main(int argc, char *argv[])
{
#ifdef GLOBAL_TRY_CATCH
    try{
#endif

    QApplication app(argc, argv);
    QApplication::setOrganizationName(AppCompany);
    //QApplication::setApplicationName(AppProduct);
    QApplication::setApplicationDisplayName(AppDisplayName);
    QApplication::setApplicationVersion(AppVersion);

    MeetingSession::locateAppdata();

    setupLogger();

    qDebug() << QApplication::applicationDisplayName()
             << "Version:" << QApplication::applicationVersion()
             << "Built:" << AppBuildDate
             << "Website: " << AppProjectWebSite;
    qDebug() << "Qt:" << QT_VERSION_STR;
    qDebug() << "Sqlite:" << sqlite3_libversion() << " DB Format: V" << FormatVersion;
    qDebug() << QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm");

    //Check SQLite thread safety
    int val = sqlite3_threadsafe();
    if(val != 1)
    {
        //Not thread safe
        qWarning() << "SQLite Library was not compiled with SQLITE_THREADSAFE=1. This may cause crashes of this application.";
    }

    MeetingSession meetingSession;
    utils::language::loadTranslationsFromSettings();

    MainWindow w;
    w.showNormal();
    w.resize(800, 600);
    w.showMaximized();

    if(argc > 1) //FIXME: better handling if there are extra arguments
    {
        QString fileName = app.arguments().at(1);
        qDebug() << "Trying to load:" << fileName;
        if(QFile(fileName).exists())
        {
            w.loadFile(app.arguments().at(1));
        }
    }

    qDebug() << "Running...";

    int ret = app.exec();
    QThreadPool::globalInstance()->waitForDone(1000);
    DB_Error err = Session->closeDB();

    if(err == DB_Error::DbBusyWhenClosing || QThreadPool::globalInstance()->activeThreadCount() > 0)
    {
        qWarning() << "Error: Application closing while threadpool still running or database busy!";
        QThreadPool::globalInstance()->waitForDone(10000);
        Session->closeDB();
    }

    return ret;


#ifdef GLOBAL_TRY_CATCH
    }catch(const char* str)
    {
        qDebug() << "Exception:" << str;
        throw;
    }catch(const std::string& str)
    {
        qDebug() << "Exception:" << str.c_str();
        throw;
    }catch(sqlite3pp::database_error& e)
    {
        qDebug() << "Exception:" << e.what();
        throw;
    }catch(std::exception& e)
    {
        qDebug() << "Exception:" << e.what();
        throw;
    }catch(...)
    {
        qDebug() << "Caught generic exception";
        throw;
    }
#endif
}
