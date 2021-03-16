#ifndef LOADSQLITETASK_H
#define LOADSQLITETASK_H

#include "../loadtaskutils.h"

/* LoadSQLiteTask
 *
 * Loads rollingstock pieces/models/owners from
 * another TrainTimetable Session (*ttt, *.db, other SQLite extensions)
 */
class LoadSQLiteTask : public ILoadRSTask
{
public:
    //5 steps
    //Load DB, Load Owners, Load Models, Load RS, Unselect Owners
    enum { StepSize = 100, MaxProgress = 5 * StepSize };

    LoadSQLiteTask(sqlite3pp::database &db, int mode, const QString& fileName, QObject *receiver);

    void run() override;

private:
    void endWithDbError(const QString& text);

    inline int calcProgress() const { return currentProgress + localProgress / localCount * StepSize; }

    bool attachDB();
    bool copyOwners();
    bool copyModels();
    bool copyRS();
    bool unselectOwnersWithNoRS();

private:
    const int importMode;
    int currentProgress;
    int localCount;
    int localProgress;
};

#endif // LOADSQLITETASK_H
