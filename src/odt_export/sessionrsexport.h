#ifndef SESSIONRSEXPORT_H
#define SESSIONRSEXPORT_H

#include "common/odtdocument.h"

#include "utils/types.h"
#include "utils/session_rs_modes.h" //TODO: complete

class SessionRSExport
{
public:
    SessionRSExport(SessionRSMode mode, SessionRSOrder order);

    void write();
    void save(const QString& fileName);

private:
    OdtDocument odt;

    SessionRSMode m_mode;
    SessionRSOrder m_order;
};

#endif // SESSIONRSEXPORT_H
