#ifndef RSIMPORTBACKEND_H
#define RSIMPORTBACKEND_H

#include <QString>
#include <QVariant>

#include "utils/types.h"

class QObject;
class IOptionsWidget;
class ILoadRSTask;

namespace sqlite3pp {
class database;
}

class RSImportBackend
{
public:
    RSImportBackend();

    virtual QString getBackendName() = 0;

    virtual IOptionsWidget *createOptionsWidget() = 0;

    virtual ILoadRSTask *createLoadTask(const QMap<QString, QVariant>& arguments, sqlite3pp::database &db,
                                        int mode, int defSpeed, RsType defType,
                                        const QString& fileName, QObject *receiver) = 0;
};

#endif // RSIMPORTBACKEND_H
