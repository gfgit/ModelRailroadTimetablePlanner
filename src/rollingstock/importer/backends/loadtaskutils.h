#ifndef LOADTASKUTILS_H
#define LOADTASKUTILS_H

#include <QCoreApplication>
#include "utils/thread/iquittabletask.h"
#include "importmodes.h"

namespace sqlite3pp {
class database;
}

class ILoadRSTask : public IQuittableTask
{
public:
    ILoadRSTask(sqlite3pp::database &db, const QString& fileName, QObject *receiver);

    inline QString getErrorText() const { return errText; }

protected:
    sqlite3pp::database &mDb;

    QString mFileName;
    QString errText;
};

class LoadTaskUtils
{
    Q_DECLARE_TR_FUNCTIONS(LoadTaskUtils)
public:
    enum { BatchSize = 50 };
};

#endif // LOADTASKUTILS_H
