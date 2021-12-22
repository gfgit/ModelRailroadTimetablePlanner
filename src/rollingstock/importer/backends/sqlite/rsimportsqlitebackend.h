#ifndef RSIMPORTSQLITEBACKEND_H
#define RSIMPORTSQLITEBACKEND_H

#include "../rsimportbackend.h"

class RSImportSQLiteBackend : public RSImportBackend
{
public:
    RSImportSQLiteBackend();

    QString getBackendName() override;

    IOptionsWidget *createOptionsWidget() override;

    ILoadRSTask *createLoadTask(const QMap<QString, QVariant>& arguments, sqlite3pp::database &db,
                                int mode, int defSpeed, RsType defType,
                                const QString& fileName, QObject *receiver) override;
};

#endif // RSIMPORTSQLITEBACKEND_H
