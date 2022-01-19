#include "shiftsheetexport.h"

#include "common/jobwriter.h"

#include "common/odtutils.h"

#include "app/session.h"

#include <sqlite3pp/sqlite3pp.h>
using namespace sqlite3pp;

#include "db_metadata/metadatamanager.h"
#include "db_metadata/imagemetadata.h"

#include <QImage>

#include <QImageReader>

#include <QDebug>

ShiftSheetExport::ShiftSheetExport(sqlite3pp::database &db, db_id shiftId) :
    mDb(db),
    m_shiftId(shiftId),
    logoWidthCm(0),
    logoHeightCm(0)
{

}

void ShiftSheetExport::write()
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
    writeGraphicsStyle(odt.stylesXml);
    writeCommonStyles(odt.stylesXml);
    JobWriter::writeJobStyles(odt.stylesXml);
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
    JobWriter::writeJobAutomaticStyles(odt.contentXml);

    bool hasLogo = (meta->hasKey(MetaDataKey::MeetingLogoPicture) == MetaDataKey::ValueFound);
    if(hasLogo)
    {
        //Save image
        saveLogoPicture();
    }
    writeCoverStyles(odt.contentXml, hasLogo);

    //Body
    odt.startBody();

    QString shiftName;

    query q(mDb, "SELECT name FROM jobshifts WHERE id=?");
    q.bind(1, m_shiftId);
    if(q.step() != SQLITE_ROW)
        qWarning() << "ShiftSheetExport: shift does not exist, id" << m_shiftId;

    shiftName = q.getRows().get<QString>(0);

    odt.setTitle(Odt::text(Odt::shiftDocTitle).arg(shiftName));

    writeCover(odt.contentXml, shiftName, hasLogo);

    JobWriter w(mDb);

    q.prepare("SELECT jobs.id,jobs.category,MIN(s1.arrival)"
              " FROM jobs"
              " JOIN stops s1 ON s1.job_id=jobs.id"
              " WHERE jobs.shift_id=?"
              " GROUP BY jobs.id"
              " ORDER BY s1.arrival ASC");
    q.bind(1, m_shiftId);
    for(auto r : q)
    {
        db_id jobId = r.get<db_id>(0);
        JobCategory cat = JobCategory(r.get<int>(1));
        w.writeJob(odt.contentXml, jobId, cat);
    }

    odt.endDocument();
}

void ShiftSheetExport::save(const QString &fileName)
{
    odt.saveTo(fileName);
}

void ShiftSheetExport::writeCoverStyles(QXmlStreamWriter &xml, bool hasImage)
{
    if(hasImage)
    {
        /* Style logo_style
         *
         * type:           graphic
         * vertical-pos:   top
         * horizontal-pos: center
         *
         * Usages:
         * - ShiftSheetExport: cover page logo picture frame
         */
        xml.writeStartElement("style:style");
        xml.writeAttribute("style:family", "graphic");
        xml.writeAttribute("style:name", "logo_style");
        xml.writeAttribute("style:parent-style-name", "Graphics");

        xml.writeStartElement("style:graphic-properties");
        xml.writeAttribute("style:vertical-rel", "paragraph");
        xml.writeAttribute("style:vertical-pos", "top");
        xml.writeAttribute("style:horizontal-rel", "paragraph");
        xml.writeAttribute("style:horizontal-pos", "center");
        xml.writeAttribute("style:wrap", "none");
        xml.writeAttribute("draw:color-mode", "standard");
        xml.writeAttribute("draw:color-inversion", "false");
        xml.writeAttribute("draw:red", "0%");
        xml.writeAttribute("draw:green", "0%");
        xml.writeAttribute("draw:blue", "0%");
        xml.writeAttribute("draw:contrast", "0%");
        xml.writeAttribute("draw:gamma", "100%");
        xml.writeAttribute("draw:luminance", "0%");
        xml.writeAttribute("draw:image-opacity", "100%");
        xml.writeAttribute("fo:clip", "rect(0cm, 0cm, 0cm, 0cm)");
        xml.writeEndElement(); //style:graphic-properties
        xml.writeEndElement(); //style
    }

    /* Style P7
     * type:        paragraph
     * text-align:  center
     * font-size:   24pt
     * font-weight: bold
     * font-name:   Liberation Serif
     *
     * Usages:
     * - ShiftSheetExport: cover page for host association name
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:name", "P7");

    xml.writeStartElement("style:paragraph-properties");
    xml.writeAttribute("fo:text-align", "center");
    xml.writeAttribute("style:justify-single-word", "false");
    xml.writeEndElement(); //style:paragraph-properties

    xml.writeStartElement("style:text-properties");
    xml.writeAttribute("style:font-name", "Liberation Serif");
    xml.writeAttribute("fo:font-size", "24pt");
    xml.writeAttribute("fo:font-weight", "bold");

    xml.writeEndElement(); //style:text-properties

    xml.writeEndElement(); //style:style

    /* Style P8
     * type:        paragraph
     * text-align:  center
     * font-size:   24pt
     * font-weight: bold
     * font-name:   Liberation Sans
     *
     * Description:
     *  Like P7 but Sans Serif font
     *
     * Usages:
     * - ShiftSheetExport: cover page for location name
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:name", "P8");

    xml.writeStartElement("style:paragraph-properties");
    xml.writeAttribute("fo:text-align", "center");
    xml.writeAttribute("style:justify-single-word", "false");
    xml.writeEndElement(); //style:paragraph-properties

    xml.writeStartElement("style:text-properties");
    xml.writeAttribute("style:font-name", "Liberation Sans");
    xml.writeAttribute("fo:font-size", "24pt");
    xml.writeAttribute("fo:font-weight", "bold");

    xml.writeEndElement(); //style:text-properties

    xml.writeEndElement(); //style:style

    /* Style shift_name_style
     * type:        paragraph
     * text-align:  center
     * font-size:   20pt
     * font-weight: bold
     * padding      0.50cm left/right, 0.10cm top, 0.15cm bottom
     * border:      0.05pt solid #000000 on all sides
     * font-name:   Liberation Sans
     *
     * Usages:
     * - ShiftSheetExport: cover page for shift name box
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:name", "shift_name_style");

    xml.writeStartElement("style:paragraph-properties");
    xml.writeAttribute("fo:text-align", "center");
    xml.writeAttribute("style:justify-single-word", "false");
    xml.writeEndElement(); //style:paragraph-properties

    xml.writeStartElement("style:text-properties");
    xml.writeAttribute("style:font-name", "Liberation Sans");
    xml.writeAttribute("fo:font-size", "32pt");
    xml.writeAttribute("fo:font-weight", "bold");

    xml.writeAttribute("fo:padding-left", "0.50cm");
    xml.writeAttribute("fo:padding-right", "0.50cm");
    xml.writeAttribute("fo:padding-top", "0.10cm");
    xml.writeAttribute("fo:padding-bottom", "0.15cm");
    xml.writeAttribute("fo:border", "0.05pt solid #000000");

    xml.writeEndElement(); //style:text-properties

    xml.writeEndElement(); //style:style
}

bool ShiftSheetExport::calculateLogoSize(QIODevice *dev)
{
    QImageReader reader(dev, "PNG");
    if(!reader.canRead())
    {
        qWarning() << "ShiftSheetExport: cannot read picture," << reader.errorString();
        return false;
    }
    QImage img;
    if(!reader.read(&img))
    {
        qWarning() << "ShiftSheetExport: cannot read picture," << reader.errorString();
        return false;
    }

    const int widthPx = img.width();
    const int heightPx = img.height();

    const double dotsX = img.dotsPerMeterX();
    const double dotsY = img.dotsPerMeterY();

    logoWidthCm = 100.0 * double(widthPx) / dotsX;
    logoHeightCm = 100.0 * double(heightPx) / dotsY;

    if(logoWidthCm > 20.0 || logoWidthCm < 2.0)
    {
        //Try with 15 cm wide
        logoWidthCm = 15.0;
        logoHeightCm = 15.0 * double(heightPx)/double(widthPx);
    }

    if(logoHeightCm > 10.0)
    {
        //Maximum 10 cm tall
        logoWidthCm = 10.0 * double(widthPx)/double(heightPx);
        logoHeightCm = 10.0;
    }

    return true;
}

void ShiftSheetExport::saveLogoPicture()
{
    std::unique_ptr<ImageMetaData::ImageBlobDevice> imageIO;
    imageIO.reset(ImageMetaData::getImage(Session->m_Db, MetaDataKey::MeetingLogoPicture));
    if(!imageIO || !imageIO->open(QIODevice::ReadOnly))
    {
        qWarning() << "ShiftSheetExport: error query image," << Session->m_Db.error_msg();
        return;
    }

    if(!calculateLogoSize(imageIO.get()))
        return; //Image is not valid

    imageIO->seek(0); //Reset device

    QString fileNamePath = odt.addImage("logo.png", "image/png");
    QFile f(fileNamePath);
    if(!f.open(QFile::WriteOnly | QFile::Truncate))
    {
        qWarning() << "ShiftSheetExport: error saving image," << f.errorString();
        return;
    }

    constexpr int bufSize = 4096;
    char buf[bufSize];
    qint64 size = 0;
    while (!imageIO->atEnd() || size < 0)
    {
        size = imageIO->read(buf, bufSize);
        if(f.write(buf, size) != size)
            qWarning() << "ShiftSheetExport: error witing image," << f.errorString();
    }
    f.close();
}

void ShiftSheetExport::writeCover(QXmlStreamWriter& xml, const QString& shiftName, bool hasLogo)
{
    MetaDataManager *meta = Session->getMetaDataManager();

    //Add some space
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "P4");
    xml.writeEndElement();

    //Host association
    QString str;
    meta->getString(str, MetaDataKey::MeetingHostAssociation);
    if(!str.isEmpty())
    {
        xml.writeStartElement("text:p");
        xml.writeAttribute("text:style-name", "P7");
        xml.writeCharacters(str);
        xml.writeEndElement();
        str.clear();
    }

    //Add some space
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "P4");
    xml.writeEndElement();

    //Logo
    if(hasLogo && !qFuzzyIsNull(logoWidthCm))
    {
        xml.writeStartElement("text:p");
        xml.writeAttribute("text:style-name", "P1");

        xml.writeStartElement("draw:frame");
        xml.writeAttribute("draw:name", "meeting_logo");
        xml.writeAttribute("draw:style-name", "logo_style");
        xml.writeAttribute("text:anchor-type", "paragraph");
        xml.writeAttribute("draw:z-index", "0");

        const QString cmFmt = QStringLiteral("%1cm");
        xml.writeAttribute("svg:width", cmFmt.arg(logoWidthCm, 0, 'f', 3));
        xml.writeAttribute("svg:height", cmFmt.arg(logoHeightCm, 0, 'f', 3));

        xml.writeStartElement("draw:image");
        xml.writeAttribute("xlink:href", "Pictures/logo.png");
        xml.writeAttribute("xlink:type", "simple");
        xml.writeAttribute("xlink:show", "embed");
        xml.writeAttribute("xlink:actuate", "onLoad");
        xml.writeEndElement(); //draw:image

        xml.writeEndElement(); //draw:frame

        xml.writeEndElement(); //text:p
    }

    //Meeting dates
    qint64 showDates = 1;
    meta->getInt64(showDates, MetaDataKey::MeetingShowDates);
    if(showDates)
    {
        QDate start, end;
        qint64 tmp = 0;

        if(meta->getInt64(tmp, MetaDataKey::MeetingStartDate) == MetaDataKey::ValueFound)
        {
            start = QDate::fromJulianDay(tmp);
        }
        if(meta->getInt64(tmp, MetaDataKey::MeetingEndDate) == MetaDataKey::ValueFound)
        {
            end = QDate::fromJulianDay(tmp);
            if(!end.isValid() || end < start)
                end = start;
        }

        if(start.isValid())
        {
            xml.writeStartElement("text:p");
            xml.writeAttribute("text:style-name", "P1");
            if(start == end)
                xml.writeCharacters(start.toString("dd/MM/yyyy"));
            else
                xml.writeCharacters(Odt::text(Odt::meetingFromToShort)
                                        .arg(start.toString("dd/MM/yyyy"),
                                             end.toString("dd/MM/yyyy")));
            xml.writeEndElement();
        }
    }

    //Add some space
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "P4");
    xml.writeEndElement();

    //Location
    meta->getString(str, MetaDataKey::MeetingLocation);
    if(!str.isEmpty())
    {
        xml.writeStartElement("text:p");
        xml.writeAttribute("text:style-name", "P8");
        xml.writeCharacters(str);
        xml.writeEndElement();
        str.clear();
    }

    //Add some space
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "P4");
    xml.writeEndElement();

    //Description
    meta->getString(str, MetaDataKey::MeetingDescription);
    if(!str.isEmpty())
    {
        xml.writeStartElement("text:p");
        xml.writeAttribute("text:style-name", "P1");

        //Split in lines
        int lastIdx = 0;
        while(true)
        {
            int idx = str.indexOf('\n', lastIdx);
            QString line = str.mid(lastIdx, idx == -1 ? idx : idx - lastIdx);
            xml.writeCharacters(line.simplified());
            if(idx < 0)
                break; //Last line
            lastIdx = idx + 1;
            xml.writeEmptyElement("text:line-break");
        }
        xml.writeEndElement();
        str.clear();
    }

    //Add some space
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "P4");
    xml.writeEndElement();

    //Shift name
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "shift_name_style");
    xml.writeCharacters(Odt::text(Odt::shiftCoverTitle).arg(shiftName));
    xml.writeEndElement();
    str.clear();

    //Interruzione pagina TODO: see style 'interruzione'
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "interruzione");
    xml.writeEndElement();
}
