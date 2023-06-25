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

#include "jobsheetexport.h"

#include "common/jobwriter.h"
#include "common/odtutils.h"

#include "app/session.h"

#include <QDebug>

JobSheetExport::JobSheetExport(db_id jobId, JobCategory cat) :
    m_jobId(jobId),
    m_jobCat(cat)
{
}

void JobSheetExport::write()
{
    qDebug() << "TEMP:" << odt.dir.path();
    odt.initDocument();

    // styles.xml font declarations
    odt.stylesXml.writeStartElement("office:font-face-decls");
    writeLiberationFontFaces(odt.stylesXml);
    odt.stylesXml.writeEndElement(); // office:font-face-decls

    // Content font declarations
    odt.contentXml.writeStartElement("office:font-face-decls");
    writeLiberationFontFaces(odt.contentXml);
    odt.contentXml.writeEndElement(); // office:font-face-decls

    // Content Automatic styles
    odt.contentXml.writeStartElement("office:automatic-styles");
    JobWriter::writeJobAutomaticStyles(odt.contentXml);

    // Styles
    odt.stylesXml.writeStartElement("office:styles");
    writeCommonStyles(odt.stylesXml);
    JobWriter::writeJobStyles(odt.stylesXml);
    odt.stylesXml.writeEndElement();

    // Body
    odt.startBody();

    JobWriter w(Session->m_Db);
    w.writeJob(odt.contentXml, m_jobId, m_jobCat);

    odt.endDocument();
}

void JobSheetExport::save(const QString &fileName)
{
    odt.saveTo(fileName);
}
