#include "stationsheetexport.h"

#include "common/stationwriter.h"

#include "common/odtutils.h"

#include "app/session.h"
#include "db_metadata/metadatamanager.h"

#include <QDebug>

StationSheetExport::StationSheetExport(db_id stationId) :
    m_stationId(stationId)
{

}

void StationSheetExport::write()
{
    qDebug() << "TEMP:" << odt.dir.path();
    odt.initDocument();

    //styles.xml font declarations
    odt.stylesXml.writeStartElement("office:font-face-decls");
    writeLiberationFontFaces(odt.stylesXml);
    odt.stylesXml.writeEndElement(); //office:font-face-decls

    //Styles
    odt.stylesXml.writeStartElement("office:styles");
    writeStandardStyle(odt.stylesXml);
    writeFooterStyle(odt.stylesXml);
    odt.stylesXml.writeEndElement();

    //Automatic styles
    odt.stylesXml.writeStartElement("office:automatic-styles");
    writePageLayout(odt.stylesXml);
    odt.stylesXml.writeEndElement();

    MetaDataManager *meta = Session->getMetaDataManager();

    //Retrive header and footer: give precedence to database metadata and then fallback to global application settings
    //If the text was explicitly set to empty in metadata no header/footer will be displayed
    QString header;
    if(meta->getString(header, MetaDataKey::SheetHeaderText) != MetaDataKey::Result::ValueFound)
    {
        header = AppSettings.getSheetHeader();
    }

    QString footer;
    if(meta->getString(footer, MetaDataKey::SheetFooterText) != MetaDataKey::Result::ValueFound)
    {
        footer = AppSettings.getSheetFooter();
    }

    //Master styles
    odt.stylesXml.writeStartElement("office:master-styles");
    writeHeaderFooter(odt.stylesXml, header, footer);
    odt.stylesXml.writeEndElement();

    //Content font declarations
    odt.contentXml.writeStartElement("office:font-face-decls");
    writeLiberationFontFaces(odt.contentXml);
    odt.contentXml.writeEndElement(); //office:font-face-decls

    //Content Automatic styles
    odt.contentXml.writeStartElement("office:automatic-styles");
    writeCommonStyles(odt.contentXml);
    StationWriter::writeStationAutomaticStyles(odt.contentXml);

    //Body
    odt.startBody();

    StationWriter w(Session->m_Db);
    QString stName;
    w.writeStation(odt.contentXml, m_stationId, &stName);
    odt.setTitle(Odt::tr("%1 station").arg(stName));

    odt.endDocument();
}

void StationSheetExport::save(const QString &fileName)
{
    odt.saveTo(fileName);
}
