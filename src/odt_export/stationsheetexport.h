#ifndef STATIONSHEETEXPORT_H
#define STATIONSHEETEXPORT_H

#include "common/odtdocument.h"

#include "utils/types.h"

//TODO: implement, deprecate MeetingSession API
class StationSheetExport
{
public:
    StationSheetExport(db_id stationId);

    void write();
    void save(const QString& fileName);

private:
    OdtDocument odt;

    db_id m_stationId;
};

#endif // STATIONSHEETEXPORT_H
