#ifndef JOBSHEETEXPORT_H
#define JOBSHEETEXPORT_H

#include "common/odtdocument.h"

#include "utils/types.h"


class JobSheetExport
{
public:
    JobSheetExport(db_id jobId, JobCategory cat);

    void write();
    void save(const QString& fileName);

private:
    OdtDocument odt;

    db_id m_jobId;
    JobCategory m_jobCat;
};

#endif // JOBSHEETEXPORT_H
