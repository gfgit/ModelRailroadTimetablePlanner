#include "odtdocument.h"

#include <zip.h>

#include <QTextStream>

#include <QDebug>

#include "info.h" //Fot App constants
#include "app/session.h" //For settings
#include "db_metadata/metadatamanager.h"

#include "odtutils.h"

//content.xml
static constexpr char contentFileStr[] = "content.xml";
static constexpr QLatin1String contentFileName = QLatin1String(contentFileStr, sizeof (contentFileStr) - 1);

//styles.xml
static constexpr char stylesFileStr[] = "styles.xml";
static constexpr QLatin1String stylesFileName = QLatin1String(stylesFileStr, sizeof (stylesFileStr) - 1);

//meta.xml
static constexpr char metaFileStr[] = "meta.xml";
static constexpr QLatin1String metaFileName = QLatin1String(metaFileStr, sizeof (metaFileStr) - 1);

//META-INF/manifest.xml
static constexpr char manifestFileNameStr[] = "manifest.xml";
static constexpr QLatin1String manifestFileName = QLatin1String(manifestFileNameStr, sizeof (manifestFileNameStr) - 1);
static constexpr char metaInfPathStr[] = "/META-INF";
static constexpr QLatin1String metaInfDirPath = QLatin1String(metaInfPathStr, sizeof (metaInfPathStr) - 1);
static constexpr char manifestFilePathStr[] = "META-INF/manifest.xml";
static constexpr QLatin1String manifestFilePath = QLatin1String(manifestFilePathStr, sizeof (manifestFilePathStr) - 1);

OdtDocument::OdtDocument()
{

}

bool OdtDocument::initDocument()
{
    if(!dir.isValid())
        return false;

    content.setFileName(dir.filePath(contentFileName));
    if(!content.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    styles.setFileName(dir.filePath(stylesFileName));
    if(!styles.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    contentXml.setDevice(&content);
    stylesXml.setDevice(&styles);

    //Init content.xml
    writeStartDoc(contentXml);
    contentXml.writeStartElement("office:document-content");
    contentXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:office:1.0", "office");
    contentXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:style:1.0", "style");
    contentXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:text:1.0", "text");
    contentXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:table:1.0", "table");
    contentXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0", "fo");
    contentXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0", "svg");
    contentXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:drawing:1.0", "draw");
    contentXml.writeNamespace("http://www.w3.org/1999/xlink", "xlink");
    contentXml.writeAttribute("office:version", "1.2");

    //Init styles.xml
    writeStartDoc(stylesXml);
    stylesXml.writeStartElement("office:document-styles");
    stylesXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:office:1.0", "office");
    stylesXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:style:1.0", "style");
    stylesXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:text:1.0", "text");
    stylesXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:table:1.0", "table");
    stylesXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0", "fo");
    stylesXml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0", "svg");
    stylesXml.writeNamespace("http://www.w3.org/1999/xlink", "xlink");
    stylesXml.writeAttribute("office:version", "1.2");

    return true;
}

void OdtDocument::startBody() //TODO: start body manually, remove this function
{
    contentXml.writeEndElement(); //office:automatic-styles

    contentXml.writeStartElement("office:body");
    contentXml.writeStartElement("office:text");
}

bool OdtDocument::saveTo(const QString& fileName)
{
    int err = 0;
    zip_t *zipper = zip_open(fileName.toUtf8(), ZIP_CREATE | ZIP_TRUNCATE, &err);

    if (zipper == nullptr)
    {
        zip_error_t ziperror;
        zip_error_init_with_code(&ziperror, err);
        qDebug() << "Failed to open output file" << fileName << "Err:" << zip_error_strerror(&ziperror);
        zip_error_fini(&ziperror);
        return false;
    }

    //Add mimetype file NOTE: must be the first file in archive
    const char mimetype[] = "application/vnd.oasis.opendocument.text";
    zip_source_t *source = zip_source_buffer(zipper, mimetype, sizeof (mimetype) - 1, 0);
    if (source == nullptr)
    {
        qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
    }

    if (zip_file_add(zipper, "mimetype", source, ZIP_FL_ENC_UTF_8) < 0)
    {
        zip_source_free(source);
        qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
    }

    //Add META-INF/manifest.xml
    QString fileToCompress = dir.filePath(manifestFilePath);

    source = zip_source_file(zipper, fileToCompress.toUtf8(), 0, 0);
    if (source == nullptr)
    {
        qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
    }

    if (zip_file_add(zipper, manifestFilePath.data(), source, ZIP_FL_ENC_UTF_8) < 0)
    {
        zip_source_free(source);
        qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
    }

    //Add styles.xml
    fileToCompress = dir.filePath(stylesFileName);

    source = zip_source_file(zipper, fileToCompress.toUtf8(), 0, 0);
    if (source == nullptr)
    {
        qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
    }

    if (zip_file_add(zipper, stylesFileName.data(), source, ZIP_FL_ENC_UTF_8) < 0)
    {
        zip_source_free(source);
        qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
    }

    //Add content.xml
    fileToCompress = dir.filePath(contentFileName);

    source = zip_source_file(zipper, fileToCompress.toUtf8(), 0, 0);
    if (source == nullptr)
    {
        qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
    }

    if (zip_file_add(zipper, contentFileName.data(), source, ZIP_FL_ENC_UTF_8) < 0)
    {
        zip_source_free(source);
        qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
    }

    //Add meta.xml
    fileToCompress = dir.filePath(metaFileName);

    source = zip_source_file(zipper, fileToCompress.toUtf8(), 0, 0);
    if (source == nullptr)
    {
        qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
    }

    if (zip_file_add(zipper, metaFileName.data(), source, ZIP_FL_ENC_UTF_8) < 0)
    {
        zip_source_free(source);
        qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
    }

    //Add possible images
    QString imgBasePath = dir.filePath("Pictures") + QLatin1String("/%1");
    QString imgNewBasePath = QLatin1String("Pictures/%1");
    for(const auto& img : qAsConst(imageList))
    {
        source = zip_source_file(zipper, imgBasePath.arg(img.first).toUtf8(), 0, 0);
        if (source == nullptr)
        {
            qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
        }

        if (zip_file_add(zipper, imgNewBasePath.arg(img.first).toUtf8(), source, ZIP_FL_ENC_UTF_8) < 0)
        {
            zip_source_free(source);
            qDebug() << "Failed to add file to zip:" << zip_strerror(zipper);
        }
    }

    zip_close(zipper);
    return true;
}

void OdtDocument::endDocument()
{
    saveManifest(dir.path());
    saveMeta(dir.path());

    contentXml.writeEndDocument();
    content.close();

    stylesXml.writeEndDocument();
    styles.close();
}

QString OdtDocument::addImage(const QString &name, const QString &mediaType)
{
    if(imageList.isEmpty())
    {
        //First image added, create Pictures folder
        QDir pictures(dir.path());
        if(!pictures.mkdir("Pictures"))
            qWarning() << "OdtDocument: cannot create Pictures folder";
    }
    imageList.append({name, mediaType});
    return dir.filePath("Pictures/" + name);
}

void OdtDocument::writeStartDoc(QXmlStreamWriter &xml)
{
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(-1);
    //xml.writeStartDocument(QStringLiteral("1.0"), true);
    xml.writeStartDocument(QStringLiteral("1.0"));
}

void OdtDocument::writeFileEntry(QXmlStreamWriter& xml,
                                 const QString& fullPath, const QString& mediaType)
{
    xml.writeStartElement("manifest:file-entry");
    xml.writeAttribute("manifest:full-path", fullPath);
    xml.writeAttribute("manifest:media-type", mediaType);
    xml.writeEndElement();
}

void OdtDocument::saveManifest(const QString& path)
{
    const QString xmlMime = QLatin1String("text/xml");

    QDir manifestDir(path + metaInfDirPath);
    if(!manifestDir.exists())
        manifestDir.mkpath(".");

    QFile manifest(dir.filePath(manifestFileName));
    manifest.open(QFile::WriteOnly | QFile::Truncate);
    QXmlStreamWriter xml(&manifest);
    writeStartDoc(xml);

    xml.writeStartElement("manifest:manifest");
    xml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:manifest:1.0", "manifest");
    xml.writeAttribute("manifest:version", "1.2");

    //Root
    writeFileEntry(xml, "/", "application/vnd.oasis.opendocument.text");

    //styles.xml
    writeFileEntry(xml, stylesFileName, xmlMime);

    //content.xml
    writeFileEntry(xml, contentFileName, xmlMime);

    //meta.xml
    writeFileEntry(xml, metaFileName, xmlMime);

    //Add possible images
    for(const auto& img : qAsConst(imageList))
    {
        writeFileEntry(xml, "Pictures/" + img.first, img.second);
    }

    xml.writeEndElement(); //manifest:manifest

    xml.writeEndDocument();
}

void OdtDocument::saveMeta(const QString& path)
{
    QDir metaDir(path);

    QFile metaFile(metaDir.filePath(metaFileName));
    metaFile.open(QFile::WriteOnly | QFile::Truncate);
    QXmlStreamWriter xml(&metaFile);
    writeStartDoc(xml);

    xml.writeStartElement("office:document-meta");
    xml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:office:1.0", "office");
    xml.writeNamespace("http://www.w3.org/1999/xlink", "xlink");
    xml.writeNamespace("http://purl.org/dc/elements/1.1/", "dc");
    xml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:meta:1.0", "meta");
    xml.writeNamespace("urn:oasis:names:tc:opendocument:xmlns:manifest:1.0", "manifest");
    xml.writeAttribute("office:version", "1.2");

    xml.writeStartElement("office:meta");

    MetaDataManager *meta = Session->getMetaDataManager();
    const bool storeLocationAndDate = AppSettings.getSheetStoreLocationDateInMeta();

    //Title
    if(!documentTitle.isEmpty())
    {
        xml.writeStartElement("dc:title");
        xml.writeCharacters(documentTitle);
        xml.writeEndElement(); //dc:title
    }

    //Subject
    xml.writeStartElement("dc:subject");
    xml.writeCharacters(AppDisplayName);
    xml.writeCharacters(" Session Meeting"); //Do not translate, so it's standard for everyone
    xml.writeEndElement(); //dc:subject

    //Description
    QString meetingLocation;
    if(storeLocationAndDate)
    {
        meta->getString(meetingLocation, MetaDataKey::MeetingLocation);

        QDate start, end;
        qint64 tmp = 0;
        if(meta->getInt64(tmp, MetaDataKey::MeetingStartDate) == MetaDataKey::ValueFound)
            start = QDate::fromJulianDay(tmp);
        if(meta->getInt64(tmp, MetaDataKey::MeetingEndDate) == MetaDataKey::ValueFound)
            end = QDate::fromJulianDay(tmp);
        if(!end.isValid() || end < start)
            end = start;

        if(!meetingLocation.isEmpty() && start.isValid())
        {
            //Store description only if metadata is valid
            //Example: Meeting in CORNUDA from 07/11/2020 to 09/11/2020

            QString description;
            if(start != end)
            {
                description = Odt::tr("Meeting in %1 from %2 to %3")
                        .arg(meetingLocation)
                        .arg(start.toString("dd/MM/yyyy"))
                        .arg(end.toString("dd/MM/yyyy"));
            }
            else
            {
                description = Odt::tr("Meeting in %1 on %2")
                        .arg(meetingLocation)
                        .arg(start.toString("dd/MM/yyyy"));
            }

            xml.writeStartElement("dc:description");
            xml.writeCharacters(description);
            xml.writeEndElement(); //dc:description
        }
    }

    //Language
    xml.writeStartElement("dc:language");
    xml.writeCharacters(AppSettings.getLanguage().name());
    xml.writeEndElement(); //dc:language

    //Generator
    xml.writeStartElement("meta:generator");
    xml.writeCharacters(AppProduct);
    xml.writeCharacters("/");
    xml.writeCharacters(AppVersion);
    xml.writeCharacters("-");
    xml.writeCharacters(AppBuildDate);
    xml.writeEndElement(); //meta:generator

    //Initial creator
    xml.writeStartElement("meta:initial-creator");
    xml.writeCharacters(AppDisplayName);
    xml.writeEndElement(); //meta:initial-creator

    //Creation date
    xml.writeStartElement("meta:creation-date");
    //NOTE: date must be in ISO 8601 format but without time zone offset (LibreOffice doesn't recognize it)
    //      so do not use Qt::ISODate otherwise there is the risk of adding time zone offset to string
    xml.writeCharacters(QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss"));
    xml.writeEndElement(); //meta:creation-date

    //Keywords
    xml.writeStartElement("meta:keyword");
    xml.writeCharacters(AppDisplayName);
    xml.writeEndElement(); //meta:keyword

    xml.writeStartElement("meta:keyword");
    xml.writeCharacters(AppProduct);
    xml.writeEndElement(); //meta:keyword

    xml.writeStartElement("meta:keyword");
    xml.writeCharacters(AppCompany);
    xml.writeEndElement(); //meta:keyword

    xml.writeStartElement("meta:keyword");
    xml.writeCharacters(Odt::tr("Meeting"));
    xml.writeEndElement(); //meta:keyword

    xml.writeStartElement("meta:keyword");
    xml.writeCharacters("Meeting"); //Untranslated version
    xml.writeEndElement(); //meta:keyword

    if(storeLocationAndDate && !meetingLocation.isEmpty())
    {
        xml.writeStartElement("meta:keyword");
        xml.writeCharacters(meetingLocation);
        xml.writeEndElement(); //meta:keyword
    }

    //End
    xml.writeEndElement(); //office:meta
    xml.writeEndElement(); //office:document-meta

    xml.writeEndDocument();
}
