#ifndef SESSIONRSWRITER_H
#define SESSIONRSWRITER_H

#include "utils/types.h"
#include "utils/session_rs_modes.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

class QXmlStreamWriter;

class SessionRSWriter
{
public:
    SessionRSWriter(database &db, SessionRSMode mode, SessionRSOrder order);

    static void writeStyles(QXmlStreamWriter &xml);

    db_id writeTable(QXmlStreamWriter& xml, const QString& parentName);

    void writeContent(QXmlStreamWriter &xml);

    QString generateTitle() const;

private:
    db_id lastParentId;

    database &mDb;

    query q_getSessionRS;
    query q_getParentName;
    query::iterator it;

    SessionRSMode m_mode;
    SessionRSOrder m_order;
};

#endif // SESSIONRSWRITER_H
