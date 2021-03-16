#ifndef ODTUTILS_H
#define ODTUTILS_H

#include <QXmlStreamWriter>

#include <QCoreApplication>

//small util
void writeColumnStyle(QXmlStreamWriter& xml, const QString& name, const QString& width);

void writeCell(QXmlStreamWriter &xml,
               const QString& cellStyle,
               const QString& paragraphStyle,
               const QString& text);


void writeCellListStart(QXmlStreamWriter &xml, const QString &cellStyle, const QString &paragraphStyle);
void writeCellListEnd(QXmlStreamWriter &xml);

void writeStandardStyle(QXmlStreamWriter &xml);

void writeGraphicsStyle(QXmlStreamWriter &xml);

void writeCommonStyles(QXmlStreamWriter &xml);

void writeFooterStyle(QXmlStreamWriter &xml);

void writePageLayout(QXmlStreamWriter &xml);

void writeHeaderFooter(QXmlStreamWriter &xml,
                       const QString& headerText,
                       const QString& footerText);

inline void writeFontFace(QXmlStreamWriter &xml,
                          const QString& name, const QString& family,
                          const QString& genericFamily, const QString& pitch)
{
    xml.writeStartElement("style:font-face");
    xml.writeAttribute("style:name", name);
    if(!family.isEmpty())
    {
        QString familyQuoted;
        familyQuoted.reserve(family.size() + 2);
        bool needsQuotes = family.contains(' '); //If family name contains blanks
        if(needsQuotes) //Enclose in single quotes
            familyQuoted.append('\'');
        familyQuoted.append(family);
        if(needsQuotes)
            familyQuoted.append('\'');
        xml.writeAttribute("svg:font-family", familyQuoted);
    }
    if(!genericFamily.isEmpty())
    {
        xml.writeAttribute("svg:font-family-generic", genericFamily);
    }
    if(!pitch.isEmpty())
    {
        xml.writeAttribute("svg:font-pitch", pitch);
    }
    xml.writeEndElement(); //style:font-face
}

void writeLiberationFontFaces(QXmlStreamWriter &xml);

class Odt
{
    Q_DECLARE_TR_FUNCTIONS(Odt)
public:
    static constexpr const char *CoupledAbbr = QT_TRANSLATE_NOOP("Odt", "Cp:");
    static constexpr const char *UncoupledAbbr = QT_TRANSLATE_NOOP("Odt", "Unc:");
};

#endif // ODTUTILS_H
