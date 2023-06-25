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

#ifndef ODTUTILS_H
#define ODTUTILS_H

#include <QXmlStreamWriter>

#include <QCoreApplication>

// small util
void writeColumnStyle(QXmlStreamWriter &xml, const QString &name, const QString &width);

void writeCell(QXmlStreamWriter &xml, const QString &cellStyle, const QString &paragraphStyle,
               const QString &text);

void writeCellListStart(QXmlStreamWriter &xml, const QString &cellStyle,
                        const QString &paragraphStyle);
void writeCellListEnd(QXmlStreamWriter &xml);

void writeStandardStyle(QXmlStreamWriter &xml);

void writeGraphicsStyle(QXmlStreamWriter &xml);

void writeCommonStyles(QXmlStreamWriter &xml);

void writeFooterStyle(QXmlStreamWriter &xml);

void writePageLayout(QXmlStreamWriter &xml);

void writeHeaderFooter(QXmlStreamWriter &xml, const QString &headerText, const QString &footerText);

inline void writeFontFace(QXmlStreamWriter &xml, const QString &name, const QString &family,
                          const QString &genericFamily, const QString &pitch)
{
    xml.writeStartElement("style:font-face");
    xml.writeAttribute("style:name", name);
    if (!family.isEmpty())
    {
        QString familyQuoted;
        familyQuoted.reserve(family.size() + 2);
        bool needsQuotes = family.contains(' '); // If family name contains blanks
        if (needsQuotes)                         // Enclose in single quotes
            familyQuoted.append('\'');
        familyQuoted.append(family);
        if (needsQuotes)
            familyQuoted.append('\'');
        xml.writeAttribute("svg:font-family", familyQuoted);
    }
    if (!genericFamily.isEmpty())
    {
        xml.writeAttribute("svg:font-family-generic", genericFamily);
    }
    if (!pitch.isEmpty())
    {
        xml.writeAttribute("svg:font-pitch", pitch);
    }
    xml.writeEndElement(); // style:font-face
}

void writeLiberationFontFaces(QXmlStreamWriter &xml);

class Odt
{
    Q_DECLARE_TR_FUNCTIONS(Odt)
public:
    struct Text
    {
        const char *sourceText;
        const char *disambiguation;
    };

    static QString text(const Text &t);

public:
    // Header/Footer
    static constexpr Text headerPage = QT_TRANSLATE_NOOP3(
      "Odt", "Page ", "Header page, leave space at end, page number will be added");

    // Meeting
    static constexpr Text meeting       = QT_TRANSLATE_NOOP3("Odt", "Meeting", "Document keywords");
    static constexpr Text meetingFromTo = QT_TRANSLATE_NOOP3(
      "Odt", "Meeting in %1 from %2 to %3", "Document description, where, from date, to date");
    static constexpr Text meetingOnDate =
      QT_TRANSLATE_NOOP3("Odt", "Meeting in %1 on %2", "Document description, where, when");
    static constexpr Text meetingFromToShort =
      QT_TRANSLATE_NOOP3("Odt", "From %1 to %2", "Shift cover, meeting from date, to date");

    // Rollingstock
    static constexpr Text CoupledAbbr = QT_TRANSLATE_NOOP3("Odt", "Cp:", "Job stop coupled RS");
    static constexpr Text UncoupledAbbr =
      QT_TRANSLATE_NOOP3("Odt", "Unc:", "Job stop uncoupled RS");
    static constexpr Text genericRSOwner = QT_TRANSLATE_NOOP3("Odt", "Owner", "Rollingstock Owner");

    static constexpr Text rsSessionTitle =
      QT_TRANSLATE_NOOP3("Odt", "Rollingstock by %1 at %2 of session",
                         "Rollingstock Session title, 1 is Owner/Station and 2 is start/end");
    static constexpr Text rsSessionStart =
      QT_TRANSLATE_NOOP3("Odt", "start", "Rollingstock Session start");
    static constexpr Text rsSessionEnd =
      QT_TRANSLATE_NOOP3("Odt", "end", "Rollingstock Session end");

    // Job Summary
    static constexpr Text jobSummaryFrom = QT_TRANSLATE_NOOP3("Odt", "From:", "Job summary");
    static constexpr Text jobSummaryTo   = QT_TRANSLATE_NOOP3("Odt", "To:", "Job summary");
    static constexpr Text jobSummaryDep  = QT_TRANSLATE_NOOP3("Odt", "Departure:", "Job summary");
    static constexpr Text jobSummaryArr  = QT_TRANSLATE_NOOP3("Odt", "Arrival:", "Job summary");
    static constexpr Text jobSummaryAxes = QT_TRANSLATE_NOOP3("Odt", "Axes:", "Job summary");

    // Job Stops Header
    static constexpr Text station = QT_TRANSLATE_NOOP3("Odt", "Station", "Job stop table");
    static constexpr Text stationPageTitle =
      QT_TRANSLATE_NOOP3("Odt", "Station: %1", "Station title in station sheet");
    static constexpr Text stationDocTitle =
      QT_TRANSLATE_NOOP3("Odt", "%1 station", "Station sheet title in document metadata");
    static constexpr Text rollingstock =
      QT_TRANSLATE_NOOP3("Odt", "Rollingstock", "Job stop table");
    static constexpr Text jobNr        = QT_TRANSLATE_NOOP3("Odt", "Job Nr", "Job column");

    static constexpr Text arrival      = QT_TRANSLATE_NOOP3("Odt", "Arrival", "Job stop table");
    static constexpr Text departure    = QT_TRANSLATE_NOOP3("Odt", "Departure", "Job stop table");
    static constexpr Text arrivalShort = QT_TRANSLATE_NOOP3("Odt", "Arr.", "Arrival abbreviated");
    static constexpr Text departureShort =
      QT_TRANSLATE_NOOP3("Odt", "Dep.", "Departure abbreviated");

    static constexpr Text stationFromCol =
      QT_TRANSLATE_NOOP3("Odt", "From", "Station stop table, From previous station column");
    static constexpr Text stationToCol =
      QT_TRANSLATE_NOOP3("Odt", "To", "Station stop table, To next station column");

    static constexpr Text jobStopPlatf =
      QT_TRANSLATE_NOOP3("Odt", "Platf", "Job stop table, platform abbreviated");
    static constexpr Text jobStopIsTransit =
      QT_TRANSLATE_NOOP3("Odt", "Transit", "Job stop table, notes column");
    static constexpr Text jobStopIsFirst =
      QT_TRANSLATE_NOOP3("Odt", "START",
                         "Station stop table, notes column for first job stop, "
                         "keep in English it's the same for every sheet");

    static constexpr Text jobStopCross = QT_TRANSLATE_NOOP3("Odt", "Crossings", "Job stop table");
    static constexpr Text jobStopPassings = QT_TRANSLATE_NOOP3("Odt", "Passings", "Job stop table");
    static constexpr Text jobStopCrossShort =
      QT_TRANSLATE_NOOP3("Odt", "Cross", "Job stop crossings abbreviated column");
    static constexpr Text jobStopPassingsShort =
      QT_TRANSLATE_NOOP3("Odt", "Passes", "Job stop passings abbreviated column");
    static constexpr Text notes = QT_TRANSLATE_NOOP3("Odt", "Notes", "Job stop table");

    // Job stops
    static constexpr Text jobReverseDirection =
      QT_TRANSLATE_NOOP3("Odt", "Reverse direction", "Job stop table");

    // Shift
    static constexpr Text shiftCoverTitle =
      QT_TRANSLATE_NOOP3("Odt", "SHIFT %1", "Shift title in shift sheet cover");
    static constexpr Text shiftDocTitle =
      QT_TRANSLATE_NOOP3("Odt", "Shift %1", "Shift sheet document title for metadata");
};

#endif // ODTUTILS_H
