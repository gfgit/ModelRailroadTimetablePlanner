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
