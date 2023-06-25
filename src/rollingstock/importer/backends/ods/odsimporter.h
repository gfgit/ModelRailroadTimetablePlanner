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

#ifndef ODSIMPORTER_H
#define ODSIMPORTER_H

#include <sqlite3pp/sqlite3pp.h>

#include "utils/types.h"

class QXmlStreamReader;
typedef struct sqlite3_mutex sqlite3_mutex;

class ODSImporter
{
public:
    ODSImporter(const int mode, const int firstRow, const int numColm, const int nameCol,
                int defSpeed, RsType defType, sqlite3pp::database &db);

    bool loadDocument(QXmlStreamReader &xml);
    bool readNextTable(QXmlStreamReader &xml);

private:
    void readTable(QXmlStreamReader &xml);
    void readRow(QXmlStreamReader &xml, QByteArray &model, qint64 &number);
    QString readCell(QXmlStreamReader &xml);

private:
    sqlite3pp::database &mDb;
    sqlite3_mutex *mutex;
    sqlite3pp::command q_addOwner;
    sqlite3pp::command q_unsetImported;
    sqlite3pp::command q_addModel;
    sqlite3pp::command q_addRS;

    sqlite3pp::query q_findOwner;
    sqlite3pp::query q_findModel;
    sqlite3pp::query q_findImportedModel;

    const int tableFirstRow;     // Start from 1 (not 0)
    const int tableRSNumberCol;  // Start from 1 (not 0)
    const int tableModelNameCol; // Start from 1 (not 0)
    const int importMode;

    int sheetIdx;
    int row;
    int col;

    int defaultSpeed;
    RsType defaultType;
};

#endif // ODSIMPORTER_H
