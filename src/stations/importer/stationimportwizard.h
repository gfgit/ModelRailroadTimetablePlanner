/*
 * ModelRailroadTimetablePlanner
 * Copyright 2016-2023, Filippo Gentile
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
    bool checkNames(db_id sourceStId, const QString& newName, QString &outShortName);
    bool addStation(db_id sourceStId, const QString& fullName, const QString &shortName);

    bool copySVGData(db_id sourceStId, db_id destStId);

private:
    sqlite3pp::database *mTempDB;
    bool mInMemory;
};

#endif // STATIONIMPORTWIZARD_H
