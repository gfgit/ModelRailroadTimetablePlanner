#include "loadtaskutils.h"


ILoadRSTask::ILoadRSTask(sqlite3pp::database &db, const QString &fileName, QObject *receiver) :
    IQuittableTask(receiver),
    mDb(db),
    mFileName(fileName)
{

}
