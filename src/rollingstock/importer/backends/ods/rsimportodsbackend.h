#ifndef RSIMPORTODSBACKEND_H
#define RSIMPORTODSBACKEND_H

#include "../rsimportbackend.h"

class RSImportODSBackend : public RSImportBackend
{
public:
    RSImportODSBackend();

    QString getBackendName() override;

    IOptionsWidget *createOptionsWidget() override;

    ILoadRSTask *createLoadTask(const QMap<QString, QVariant>& arguments, sqlite3pp::database &db,
                                int mode, int defSpeed, RsType defType,
                                const QString& fileName, QObject *receiver) override;
};

#endif // RSIMPORTODSBACKEND_H
