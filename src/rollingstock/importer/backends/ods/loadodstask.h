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

#ifndef LOADODSTASK_H
#define LOADODSTASK_H

#include "../loadtaskutils.h"

#include "utils/types.h"

/* LoadODSTask
 *
 * Loads rollingstock pieces/models/owners from an ODS spreadsheet
 * Open Document Format V1.2 (*.ods)
 *
 * Table Characteristics
 * 1) tblFirstRow: first non-empty RS row (starting from 1, not 0) DEFAULT: 3
 * 2) tblRSNumberCol: column from which number is extracted  (starting from 1, not 0) DEFAULT: 1
 * 3) tblFirstRow: column from which model name is extracted (starting from 1, not 0) DEFAULT: 3
 */
class LoadODSTask : public ILoadRSTask
{
public:
    LoadODSTask(const QMap<QString, QVariant>& arguments, sqlite3pp::database &db,
                int mode, int defSpeed, RsType defType,
                const QString& fileName, QObject *receiver);

    void run() override;

private:
    int m_tblFirstRow;     //Start from 1 (not 0)
    int m_tblRSNumberCol;  //Start from 1 (not 0)
    int m_tblModelNameCol; //Start from 1 (not 0)
    int importMode;

    int defaultSpeed;
    RsType defaultType;
};

#endif // LOADODSTASK_H
