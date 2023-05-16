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

#ifndef SHIFTSHEETEXPORT_H
#define SHIFTSHEETEXPORT_H

#include "common/odtdocument.h"

#include "utils/types.h"

namespace sqlite3pp {
class database;
}

class ShiftSheetExport
{
public:
    ShiftSheetExport(sqlite3pp::database &db, db_id shiftId);

    void write();
    void save(const QString& fileName);

    inline void setShiftId(db_id shiftId) { m_shiftId = shiftId; }

private:
    void writeCoverStyles(QXmlStreamWriter &xml, bool hasImage);
    bool calculateLogoSize(QIODevice *dev);
    void saveLogoPicture();
    void writeCover(QXmlStreamWriter &xml, const QString &shiftName, bool hasLogo);

private:
    sqlite3pp::database &mDb;
    db_id m_shiftId;

    OdtDocument odt;

    double logoWidthCm;
    double logoHeightCm;
};

#endif // SHIFTSHEETEXPORT_H
