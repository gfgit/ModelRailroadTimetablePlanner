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
    static QString text(const char * const str[2]);

public:
    //Header/Footer
    static constexpr const char *headerPage[] = QT_TRANSLATE_NOOP3("Odt", "Page ", "Header page, leave space at end, page number will be added");

    //Meeting
    static constexpr const char *meeting[]       = QT_TRANSLATE_NOOP3("Odt", "Meeting", "Document keywords");
    static constexpr const char *meetingFromTo[] = QT_TRANSLATE_NOOP3("Odt", "Meeting in %1 from %2 to %3", "Document description, where, from date, to date");
    static constexpr const char *meetingOnDate[] = QT_TRANSLATE_NOOP3("Odt", "Meeting in %1 on %2",         "Document description, where, when");
    static constexpr const char *meetingFromToShort[] = QT_TRANSLATE_NOOP3("Odt", "From %1 to %2", "Shift cover, meeting from date, to date");

    //Rollingstock
    static constexpr const char *CoupledAbbr[]    = QT_TRANSLATE_NOOP3("Odt", "Cp:",  "Job stop coupled RS");
    static constexpr const char *UncoupledAbbr[]  = QT_TRANSLATE_NOOP3("Odt", "Unc:", "Job stop uncoupled RS");
    static constexpr const char *genericRSOwner[] = QT_TRANSLATE_NOOP3("Odt", "Owner", "Rollingstock Owner");

    static constexpr const char *rsSessionTitle[] = QT_TRANSLATE_NOOP3("Odt", "Rollingstock by %1 at %2 of session",
                                                                       "Rollingstock Session title, 1 is Owner/Station and 2 is start/end");
    static constexpr const char *rsSessionStart[] = QT_TRANSLATE_NOOP3("Odt", "start", "Rollingstock Session start");
    static constexpr const char *rsSessionEnd[]   = QT_TRANSLATE_NOOP3("Odt", "end", "Rollingstock Session end");

    //Job Summary
    static constexpr const char *jobSummaryFrom[] = QT_TRANSLATE_NOOP3("Odt", "From:",      "Job summary");
    static constexpr const char *jobSummaryTo[]   = QT_TRANSLATE_NOOP3("Odt", "To:",        "Job summary");
    static constexpr const char *jobSummaryDep[]  = QT_TRANSLATE_NOOP3("Odt", "Departure:", "Job summary");
    static constexpr const char *jobSummaryArr[]  = QT_TRANSLATE_NOOP3("Odt", "Arrival:",   "Job summary");
    static constexpr const char *jobSummaryAxes[] = QT_TRANSLATE_NOOP3("Odt", "Axes:",      "Job summary");

    //Job Stops Header
    static constexpr const char *station[]      = QT_TRANSLATE_NOOP3("Odt", "Station",      "Job stop table");
    static constexpr const char *stationPageTitle[] = QT_TRANSLATE_NOOP3("Odt", "Station: %1",  "Station title in station sheet");
    static constexpr const char *stationDocTitle[]  = QT_TRANSLATE_NOOP3("Odt", "%1 station",  "Station sheet title in document metadata");
    static constexpr const char *rollingstock[] = QT_TRANSLATE_NOOP3("Odt", "Rollingstock", "Job stop table");
    static constexpr const char *jobNr[]        = QT_TRANSLATE_NOOP3("Odt", "Job Nr",       "Job column");

    static constexpr const char *arrival[]   = QT_TRANSLATE_NOOP3("Odt", "Arrival",   "Job stop table");
    static constexpr const char *departure[] = QT_TRANSLATE_NOOP3("Odt", "Departure", "Job stop table");
    static constexpr const char *arrivalShort[]   = QT_TRANSLATE_NOOP3("Odt", "Arr.", "Arrival abbreviated");
    static constexpr const char *departureShort[] = QT_TRANSLATE_NOOP3("Odt", "Dep.", "Departure abbreviated");

    static constexpr const char *stationFromCol[] = QT_TRANSLATE_NOOP3("Odt", "From", "Station stop table, From previous station column");
    static constexpr const char *stationToCol[]   = QT_TRANSLATE_NOOP3("Odt", "To",   "Station stop table, To next station column");

    static constexpr const char *jobStopPlatf[] = QT_TRANSLATE_NOOP3("Odt", "Platf", "Job stop table, platform abbreviated");
    static constexpr const char *jobStopIsTransit[] = QT_TRANSLATE_NOOP3("Odt", "Transit", "Job stop table, notes column");
    static constexpr const char *jobStopIsFirst[] = QT_TRANSLATE_NOOP3("Odt", "START",
                                                                       "Station stop table, notes column for first job stop, "
                                                                       "keep in English it's the same for every sheet");

    static constexpr const char *jobStopCross[]    = QT_TRANSLATE_NOOP3("Odt", "Crossings",  "Job stop table");
    static constexpr const char *jobStopPassings[] = QT_TRANSLATE_NOOP3("Odt", "Passings", "Job stop table");
    static constexpr const char *jobStopCrossShort[]    = QT_TRANSLATE_NOOP3("Odt", "Cross",  "Job stop crossings abbreviated column");
    static constexpr const char *jobStopPassingsShort[] = QT_TRANSLATE_NOOP3("Odt", "Passes", "Job stop passings abbreviated column");
    static constexpr const char *notes[] = QT_TRANSLATE_NOOP3("Odt", "Notes", "Job stop table");

    //Shift
    static constexpr const char *shiftCoverTitle[] = QT_TRANSLATE_NOOP3("Odt", "SHIFT %1", "Shift title in shift sheet cover");
    static constexpr const char *shiftDocTitle[]   = QT_TRANSLATE_NOOP3("Odt", "Shift %1", "Shift sheet document title for metadata");
};

#endif // ODTUTILS_H
