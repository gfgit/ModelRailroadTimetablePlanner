#include "rsimportsqlitebackend.h"

#include "sqliteoptionswidget.h"
#include "loadsqlitetask.h"

#include "utils/files/file_format_names.h"

RSImportSQLiteBackend::RSImportSQLiteBackend()
{

}

QString RSImportSQLiteBackend::getBackendName()
{
    return FileFormats::tr(FileFormats::tttFormat);
}

IOptionsWidget *RSImportSQLiteBackend::createOptionsWidget()
{
    return new SQLiteOptionsWidget(nullptr);
}

ILoadRSTask *RSImportSQLiteBackend::createLoadTask(const QMap<QString, QVariant> &arguments, sqlite3pp::database &db,
                                                int mode, int defSpeed, RsType defType,
                                                const QString &fileName, QObject *receiver)
{
    Q_UNUSED(arguments)
    Q_UNUSED(defSpeed)
    Q_UNUSED(defType)
    return new LoadSQLiteTask(db, mode, fileName, receiver);
}
