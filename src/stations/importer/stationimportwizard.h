#ifndef STATIONIMPORTWIZARD_H
#define STATIONIMPORTWIZARD_H

#include <QWizard>
#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class StationImportWizard : public QWizard
{
    Q_OBJECT
public:
    enum Pages
    {
        OptionsPageIdx = 0,
        ChooseFileIdx,
        SelectStationIdx
    };

    explicit StationImportWizard(QWidget *parent = nullptr);
    ~StationImportWizard();

    inline sqlite3pp::database *getTempDB() const { return mTempDB; }

private slots:
    void onFileChosen(const QString& fileName);

private:
    bool createDatabase(bool inMemory, const QString& fileName);
    bool closeDatabase();

    friend class SelectStationPage;
    bool addStation(db_id sourceStId, const QString& newName);

private:
    sqlite3pp::database *mTempDB;
    bool mInMemory;
};

#endif // STATIONIMPORTWIZARD_H
