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

#include "sessionrsexport.h"

#include "common/odtutils.h"
#include "common/sessionrswriter.h"

#include "app/session.h"
#include "db_metadata/metadatamanager.h"

#include <QDebug>

SessionRSExport::SessionRSExport(SessionRSMode mode, SessionRSOrder order) :
    m_mode(mode),
    m_order(order)
{
}

void SessionRSExport::write()
{
    qDebug() << "TEMP:" << odt.dir.path();
    odt.initDocument();

    // styles.xml font declarations
    odt.stylesXml.writeStartElement("office:font-face-decls");
    writeLiberationFontFaces(odt.stylesXml);
    odt.stylesXml.writeEndElement(); // office:font-face-decls

    // Styles
    odt.stylesXml.writeStartElement("office:styles");
    writeStandardStyle(odt.stylesXml);
    writeCommonStyles(odt.stylesXml);
    // JobWriter::writeJobStyles(odt.stylesXml); TODO
    writeFooterStyle(odt.stylesXml);
    odt.stylesXml.writeEndElement();

    // Automatic styles
    odt.stylesXml.writeStartElement("office:automatic-styles");
    writePageLayout(odt.stylesXml);
    odt.stylesXml.writeEndElement();

    MetaDataManager *meta = Session->getMetaDataManager();

    // Retrive header and footer: give precedence to database metadata and then fallback to global
    // application settings If the text was explicitly set to empty in metadata no header/footer
    // will be displayed
    QString header;
    if (meta->getString(header, MetaDataKey::SheetHeaderText) != MetaDataKey::Result::ValueFound)
    {
        header = AppSettings.getSheetHeader();
    }

    QString footer;
    if (meta->getString(footer, MetaDataKey::SheetFooterText) != MetaDataKey::Result::ValueFound)
    {
        footer = AppSettings.getSheetFooter();
    }

    // Master styles
    odt.stylesXml.writeStartElement("office:master-styles");
    writeHeaderFooter(odt.stylesXml, header, footer);
    odt.stylesXml.writeEndElement();

    // Content font declarations
    odt.contentXml.writeStartElement("office:font-face-decls");
    writeLiberationFontFaces(odt.contentXml);
    odt.contentXml.writeEndElement(); // office:font-face-decls

    // Content Automatic styles
    odt.contentXml.writeStartElement("office:automatic-styles");
    SessionRSWriter::writeStyles(odt.contentXml);

    // Body
    odt.startBody();

    SessionRSWriter w(Session->m_Db, m_mode, m_order);
    odt.setTitle(w.generateTitle());
    w.writeContent(odt.contentXml);

    odt.endDocument();
}

void SessionRSExport::save(const QString &fileName)
{
    odt.saveTo(fileName);
}
