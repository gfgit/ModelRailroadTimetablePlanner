#ifndef SHIFTSHEETEXPORT_H
#define SHIFTSHEETEXPORT_H

#include "common/odtdocument.h"

#include "utils/types.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class ShiftSheetExport
{
public:
    ShiftSheetExport(db_id shiftId);

    void write();
    void save(const QString& fileName);

    inline void setShiftId(db_id shiftId) { m_shiftd = shiftId; }

private:
    void writeCoverStyles(QXmlStreamWriter &xml, bool hasImage);
    bool calculateLogoSize(QIODevice *dev);
    void saveLogoPicture();
    void writeCover(QXmlStreamWriter &xml, const QString &shiftName, bool hasLogo);

private:
    OdtDocument odt;

    db_id m_shiftd;

    query q_getShiftJobs;

    double logoWidthCm;
    double logoHeightCm;
};

#endif // SHIFTSHEETEXPORT_H
