#include "rsimportodsbackend.h"

#include "odsoptionswidget.h"
#include "loadodstask.h"

#include "utils/files/file_format_names.h"

RSImportODSBackend::RSImportODSBackend()
{

}

QString RSImportODSBackend::getBackendName()
{
    return FileFormats::tr(FileFormats::odsFormat);
}

IOptionsWidget *RSImportODSBackend::createOptionsWidget()
{
    return new ODSOptionsWidget;
}

ILoadRSTask *RSImportODSBackend::createLoadTask(const QMap<QString, QVariant> &arguments, sqlite3pp::database &db,
                                                int mode, int defSpeed, RsType defType,
                                                const QString &fileName, QObject *receiver)
{
    return new LoadODSTask(arguments, db, mode, defSpeed, defType, fileName, receiver);
}
