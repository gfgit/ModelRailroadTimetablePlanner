#ifndef IMPORTTASK_H
#define IMPORTTASK_H

#include "utils/thread/iquittabletask.h"

class QObject;

namespace sqlite3pp {
class database;
}

class ImportTask : public IQuittableTask
{
public:
    ImportTask(sqlite3pp::database &db, QObject *receiver);

    void run() override;

private:
    sqlite3pp::database &mDb;
};

#endif // IMPORTTASK_H
