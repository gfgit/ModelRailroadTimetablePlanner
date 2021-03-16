#include "odtutils.h"

/* writeColumnStyle
 *
 * Helper function to write table column style of a certain width
 */
void writeColumnStyle(QXmlStreamWriter &xml, const QString &name, const QString &width)
{
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "table-column");
    xml.writeAttribute("style:name", name);

    xml.writeStartElement("style:table-column-properties");
    xml.writeAttribute("style:column-width", width);
    xml.writeEndElement(); //style:table-column-properties
    xml.writeEndElement(); //style
}

/* writeCell
 *
 * Helper function to write table cell
 * Sets up the cell style and paragraph style, writes single line text and closes xml opened elements
 */
void writeCell(QXmlStreamWriter &xml, const QString &cellStyle, const QString &paragraphStyle, const QString &text)
{
    //Cell
    xml.writeStartElement("table:table-cell");
    xml.writeAttribute("office:value-type", "string");
    xml.writeAttribute("table:style-name", cellStyle);

    //text:p
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", paragraphStyle);
    xml.writeCharacters(text);
    xml.writeEndElement(); //text:p

    xml.writeEndElement(); //table-cell
}

/* writeCellListStart
 *
 * Helper function to write table cell but more advanced than writeCell
 * Sets up the cell style and paragraph style
 * Then you have full control on cell contents
 * Then call writeCellListEnd
 */
void writeCellListStart(QXmlStreamWriter &xml, const QString &cellStyle, const QString &paragraphStyle)
{
    //Cell
    xml.writeStartElement("table:table-cell");
    xml.writeAttribute("office:value-type", "string");
    xml.writeAttribute("table:style-name", cellStyle);

    //text:p
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", paragraphStyle);
}

/* writeCellListEnd
 *
 * Helper function to close cell
 */
void writeCellListEnd(QXmlStreamWriter &xml)
{
    xml.writeEndElement(); //text:p
    xml.writeEndElement(); //table-cell
}

void writeStandardStyle(QXmlStreamWriter &xml)
{
    xml.writeStartElement("style:style");

    xml.writeAttribute("style:name", "Standard");
    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:class", "text");

    xml.writeEndElement(); //style:style
}

void writeGraphicsStyle(QXmlStreamWriter &xml)
{
    xml.writeStartElement("style:style");

    xml.writeAttribute("style:name", "Graphics");
    xml.writeAttribute("style:family", "graphic");

    xml.writeStartElement("style:graphic-properties");
    xml.writeAttribute("svg:x", "0cm");
    xml.writeAttribute("svg:y", "0cm");
    xml.writeAttribute("style:vertical-pos", "top");
    xml.writeAttribute("style:vertical-rel", "paragraph");
    xml.writeAttribute("style:horizontal-pos", "center");
    xml.writeAttribute("style:horizontal-rel", "paragraph");
    xml.writeEndElement(); //style:graphic-properties

    xml.writeEndElement(); //style:style
}

void writeCommonStyles(QXmlStreamWriter &xml)
{
    /* Style P1
     * type: paragraph style
     * text-align: center
     * font-size: 18pt
     * font-weight: bold
     *
     * Usages:
     * - JobWriter:
     *    - Job name (page title)
     *    - empty line for spacing after title
     *    - empty line for spacing after job_summary
     *    - empty line for spacing after job_stops
     *
     * - SessionRSWriter
     *    - Station or Rollingstock Owner name
     *    - empty line for spacing after rollingstock table
     *
     * - ShiftSheetExport:
     *    - Logo Picture text paragraph
     *      Could be any paragraph style, we just need a valid <text:p> to draw a frame inside
     *    - Meeting dates
     *    - Meeting description
     *
     * - StationWriter:
     *    - Station name (page title)
     *    - empty line for spacing after title
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:name", "P1");

    xml.writeStartElement("style:paragraph-properties");
    xml.writeAttribute("fo:text-align", "center");
    xml.writeAttribute("style:justify-single-word", "false");
    xml.writeEndElement(); //style:paragraph-properties

    xml.writeStartElement("style:text-properties");
    xml.writeAttribute("fo:font-size", "18pt");
    xml.writeAttribute("fo:font-weight", "bold");
    xml.writeEndElement(); //style:text-properties

    xml.writeEndElement(); //style:style

    /* Style P4
     * type: paragraph style
     * text-align: center
     * font-size: 12pt
     * font-weight: bold
     *
     * Desctription:
     * - JobWriter: bold for headings
     * - StationWriter: bold and a bit bigger than P3, used to emphatize Arrival or Departure
     *
     * Usages:
     * - JobWriter:
     *    - job_stops table: Heading row cells (used for column names)
     *
     * - SessionRSWriter:
     *    - rollingstock table: Heading row cells (used for column names)
     *    - rollingstock table: content row cells
     *
     * - ShiftSheetExport
     *    - empty line for spacing at the very top of cover page (before Host Association name)
     *    - empty line for spacing before Logo Picture (after Host association)
     *    - empty line for spacing before Location (after Meeting dates)
     *    - empty line for spacing before description (after Location)
     *    - empty line for spacing before shift name box (after Description)
     *
     * - StationWriter:
     *    - stationtable table: Arrival for normal stop jobs
     *    - stationtable table: Departure on the repeated row for normal stop jobs
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:name", "P4");

    xml.writeStartElement("style:paragraph-properties");
    xml.writeAttribute("fo:text-align", "center");
    xml.writeAttribute("style:justify-single-word", "false");
    xml.writeEndElement(); //style:paragraph-properties

    xml.writeStartElement("style:text-properties");
    xml.writeAttribute("fo:font-size", "12pt");
    xml.writeAttribute("fo:font-weight", "bold");
    xml.writeEndElement(); //style:text-properties

    xml.writeEndElement(); //style:style

    /* Style T1
     * type: text style
     * font-weight: bold
     *
     * Desctription:
     * Set font to bold, inherit other properties from paragraph
     *
     * Usages:
     * - JobWriter: bold for 'Cp:' and 'Unc:' rollingstock
     * - StationWriter: bold for 'Cp:' and 'Unc:' rollingstock
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "text");
    xml.writeAttribute("style:name", "T1");

    xml.writeStartElement("style:text-properties");
    xml.writeAttribute("fo:font-weight", "bold");
    xml.writeEndElement(); //style:text-properties

    xml.writeEndElement(); //style:style
}

void writeFooterStyle(QXmlStreamWriter &xml)
{
    //Base style for MP1 style used in header/footer
    xml.writeStartElement("style:style");

    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:class", "extra");
    xml.writeAttribute("style:name", "Footer");
    xml.writeAttribute("style:parent-style-name", "Standard");

    xml.writeStartElement("style:paragraph-properties");

    xml.writeAttribute("text:line-number", "0");
    xml.writeAttribute("text:number-lines", "false");

    xml.writeStartElement("style:tab-stops");

    xml.writeStartElement("style:tab-stop");
    xml.writeAttribute("style:position", "8.5cm");
    xml.writeAttribute("style:type", "center");
    xml.writeEndElement(); //style:tab-stop

    xml.writeStartElement("style:tab-stop");
    xml.writeAttribute("style:position", "17cm");
    xml.writeAttribute("style:type", "right");
    xml.writeEndElement(); //style:tab-stop

    xml.writeEndElement(); //style:tab-stops

    xml.writeEndElement(); //style:paragraph-properties

    xml.writeEndElement(); //style:style
}

void writePageLayout(QXmlStreamWriter &xml)
{
    //Footer style
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:name", "MP1");
    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:parent-style-name", "Footer");
    xml.writeStartElement("style:text-properties");
    xml.writeAttribute("style:font-name", "Liberation Sans");
    xml.writeEndElement(); //style:text-properties
    xml.writeEndElement(); //style:style

    //Page Layout
    xml.writeStartElement("style:page-layout");

    xml.writeAttribute("style:name", "Mpm1");

    xml.writeStartElement("style:page-layout-properties");
    xml.writeAttribute("fo:margin-top", "2cm");
    xml.writeAttribute("fo:margin-bottom", "2cm");
    xml.writeAttribute("fo:margin-left", "2cm");
    xml.writeAttribute("fo:margin-right", "2cm");
    xml.writeEndElement(); //style:page-layout-properties

    xml.writeStartElement("style:header-style");
    xml.writeEndElement(); //style:header-style

    xml.writeStartElement("style:footer-style");
    xml.writeEndElement(); //style:footer-style

    xml.writeEndElement(); //style:page-layout
}

void writeHeaderFooter(QXmlStreamWriter &xml, const QString& headerText, const QString& footerText)
{
    xml.writeStartElement("style:master-page");

    xml.writeAttribute("style:name", "Standard");
    xml.writeAttribute("style:page-layout-name", "Mpm1"); //TODO

    //Header
    xml.writeStartElement("style:header");

    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "MP1");

    xml.writeCharacters(headerText);

    xml.writeStartElement("text:tab");
    xml.writeEndElement(); //text:tab

    xml.writeStartElement("text:tab");
    xml.writeEndElement(); //text:tab

    const QString pageStr = Odt::tr("Page ");

    xml.writeCharacters(pageStr);

    xml.writeStartElement("text:page-number");
    xml.writeAttribute("text:select-page", "current");
    xml.writeEndElement(); //text:page-number

    xml.writeEndElement(); //text:p

    xml.writeEndElement(); //style:header

    //Header for left pages (mirrored)
    xml.writeStartElement("style:header-left");

    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "MP1");

    xml.writeCharacters(pageStr);

    xml.writeStartElement("text:page-number");
    xml.writeAttribute("text:select-page", "current");
    xml.writeEndElement(); //text:page-number

    xml.writeStartElement("text:tab");
    xml.writeEndElement(); //text:tab

    xml.writeStartElement("text:tab");
    xml.writeEndElement(); //text:tab

    xml.writeCharacters(headerText);

    xml.writeEndElement(); //text:p

    xml.writeEndElement(); //style:header-left

    //Footer
    xml.writeStartElement("style:footer");

    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "MP1");

    xml.writeCharacters(footerText);

    xml.writeEndElement(); //text:p

    xml.writeEndElement(); //style:footer

    xml.writeEndElement(); //style:master-page
}

void writeLiberationFontFaces(QXmlStreamWriter &xml)
{
    const QString variablePitch = QStringLiteral("variable");
    const QString liberationSerif = QStringLiteral("Liberation Serif");
    const QString liberationSans = QStringLiteral("Liberation Sans");
    const QString liberationMono = QStringLiteral("Liberation Mono");

    writeFontFace(xml, liberationSerif, liberationSerif, "roman", variablePitch);
    writeFontFace(xml, liberationSans, liberationSans, "swiss", variablePitch);
    writeFontFace(xml, liberationMono, liberationMono, "modern", "fixed");
}
