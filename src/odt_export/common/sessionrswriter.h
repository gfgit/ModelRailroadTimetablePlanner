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
