#include "stationwriter.h"

#include "jobs/jobsmanager/model/jobshelper.h"

#include "utils/jobcategorystrings.h"
#include "utils/rs_utils.h"

#include <QXmlStreamWriter>

#include "odtutils.h"

#include <QDebug>

//first == true for the first time the stop is writter
//the second time it should be false to skip the repetition of Arrival, Crossings, Passings
void StationWriter::insertStop(QXmlStreamWriter &xml, const Stop& stop, bool first, bool transit)
{
    const QString P3_style = "P3";

    //Row
    xml.writeStartElement("table:table-row");

    //Cells, hide Arrival the second time
    if(first)
    {
        //Arrival in bold, if transit bold + italic
        writeCell(xml, "stationtable.A2", transit ? "P5" : "P4", stop.arrival.toString("HH:mm"));

        //Departure, if transit don't repeat it
        writeCell(xml, "stationtable.A2", P3_style, transit ? "-/-" : stop.departure.toString("HH:mm"));
    }
    else
    {
        writeCell(xml, "stationtable.A2", P3_style, QString()); //Don't repeat Arrival
        writeCell(xml, "stationtable.A2", "P4", stop.departure.toString("HH:mm")); //Departure in bold
    }

    //Job N
    writeCell(xml, "stationtable.A2", P3_style, JobCategoryName::jobName(stop.jobId, stop.jobCat));
    writeCell(xml, "stationtable.A2", P3_style, stop.platform); //Platform

    //From Previous Station, write only first time
    writeCell(xml, "stationtable.A2", P3_style, first ? stop.prevSt : QString());
    writeCell(xml, "stationtable.A2", P3_style, stop.nextSt);

    if(!first)
    {
        //Fill with empty cells, needed to keep the order
        writeCell(xml, "stationtable.A2", P3_style, QString()); //Rollingstock
        writeCell(xml, "stationtable.A2", P3_style, QString()); //Crossings
        writeCell(xml, "stationtable.A2", P3_style, QString()); //Passings

        //Notes, description, repaeat to user the arrival to better link the 2 rows
        writeCell(xml, "stationtable.L2", P3_style, stop.arrival.toString("HH:mm"));
    }
}

StationWriter::StationWriter(database &db) :
    mDb(db),
    q_getJobsByStation(mDb, "SELECT stops.id,"
                            "stops.job_id,"
                            "jobs.category,"
                            "stops.arrival,"
                            "stops.departure,"
                            "stops.type,"
                            "stops.description,"
                            "t1.name,"
                            "t2.name"
                            " FROM stops"
                            " JOIN jobs ON jobs.id=stops.job_id"
                            " LEFT JOIN station_gate_connections g1 ON g1.id=stops.in_gate_conn"
                            " LEFT JOIN station_gate_connections g2 ON g2.id=stops.out_gate_conn"
                            " LEFT JOIN station_tracks t1 ON t1.id=g1.track_id"
                            " LEFT JOIN station_tracks t2 ON t2.id=g2.track_id"
                            " WHERE stops.station_id=?"
                            " ORDER BY stops.arrival,stops.job_id"),

    q_selectPassings(mDb, "SELECT stops.id,stops.job_id,jobs.category"
                          " FROM stops"
                          " JOIN jobs ON jobs.id=stops.job_id"
                          " WHERE stops.station_id=? AND stops.departure>=? AND stops.arrival<=? AND stops.job_id<>?"),
    q_getStopCouplings(mDb, "SELECT coupling.rs_id,"
                            "rs_list.number,rs_models.name,rs_models.suffix,rs_models.type"
                            " FROM coupling"
                            " JOIN rs_list ON rs_list.id=coupling.rs_id"
                            " JOIN rs_models ON rs_models.id=rs_list.model_id"
                            " WHERE coupling.stop_id=? AND coupling.operation=?")
{

}

//TODO: common styles with JobWriter should go in common
void StationWriter::writeStationAutomaticStyles(QXmlStreamWriter &xml)
{
    /* Style: stationtable
    *
    * Type:         table
    * Display name: Station Table
    * Align:        center
    * Width:        20.0cm
    *
    * Usage:
    *  - StationWriter: station sheet main table showing jobs that stop in this station
    */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "table");
    xml.writeAttribute("style:name", "stationtable");
    xml.writeAttribute("style:display-name", "Station Table");
    xml.writeStartElement("style:table-properties");
    xml.writeAttribute("style:shadow", "none");
    xml.writeAttribute("table:align", "center");
    xml.writeAttribute("style:width", "20.0cm");
    xml.writeEndElement(); //style:table-properties
    xml.writeEndElement(); //style

    //stationtable columns
    writeColumnStyle(xml, "stationtable.A", "1.74cm"); //1  Arrival
    writeColumnStyle(xml, "stationtable.B", "1.80cm"); //2  Departure
    writeColumnStyle(xml, "stationtable.C", "1.93cm"); //3  Job N
    writeColumnStyle(xml, "stationtable.D", "1.00cm"); //4  Platform (Platf)
    writeColumnStyle(xml, "stationtable.E", "3.09cm"); //5  From (Previous Station)
    writeColumnStyle(xml, "stationtable.F", "3.11cm"); //6  To (Next Station)
    writeColumnStyle(xml, "stationtable.G", "3.00cm"); //7  Rollingstock
    writeColumnStyle(xml, "stationtable.H", "2.10cm"); //8  Crossings
    writeColumnStyle(xml, "stationtable.I", "2.10cm"); //9  Passings
    writeColumnStyle(xml, "stationtable.L", "2.21cm"); //10 Description

    /* Style: stationtable.A1
     *
     * Type: table-cell
     * Border: 0.05pt solid #000000 on left, top, bottom sides
     * Padding: 0.097cm all sides
     *
     * Usage:
     *  - stationtable table: top left/middle cells (except top right which has L1 style)
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "table-cell");
    xml.writeAttribute("style:name", "stationtable.A1");

    xml.writeStartElement("style:table-cell-properties");
    xml.writeAttribute("fo:padding", "0.097cm");
    xml.writeAttribute("fo:border-left", "0.05pt solid #000000");
    xml.writeAttribute("fo:border-right", "none");
    xml.writeAttribute("fo:border-top", "0.05pt solid #000000");
    xml.writeAttribute("fo:border-bottom", "0.05pt solid #000000");
    xml.writeEndElement(); //style:table-cell-properties
    xml.writeEndElement(); //style

    /* Style: stationtable.L1
     *
     * Type: table-cell
     * Border: 0.05pt solid #000000 on all sides
     * Padding: 0.097cm all sides
     *
     * Usage:
     *  - stationtable table: top right cell
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "table-cell");
    xml.writeAttribute("style:name", "stationtable.L1");

    xml.writeStartElement("style:table-cell-properties");
    xml.writeAttribute("fo:border", "0.05pt solid #000000");
    xml.writeAttribute("fo:padding", "0.097cm");
    xml.writeEndElement(); //style:table-cell-properties
    xml.writeEndElement(); //style

    /* Style: stationtable.A2
     *
     * Type: table-cell
     * Border: 0.05pt solid #000000 on left and bottom sides
     * Padding: 0.097cm all sides
     *
     * Usage:
     *  - stationtable table: right and middle cells from second row to last row
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "table-cell");
    xml.writeAttribute("style:name", "stationtable.A2");

    xml.writeStartElement("style:table-cell-properties");
    xml.writeAttribute("fo:padding", "0.097cm");
    xml.writeAttribute("fo:border-left", "0.05pt solid #000000");
    xml.writeAttribute("fo:border-right", "none");
    xml.writeAttribute("fo:border-top", "none");
    xml.writeAttribute("fo:border-bottom", "0.05pt solid #000000");
    xml.writeEndElement(); //style:table-cell-properties
    xml.writeEndElement(); //style

    /* Style: stationtable.L2
     *
     * Type: table-cell
     * Border: 0.05pt solid #000000 on left, right and bottom sides
     * Padding: 0.097cm all sides
     *
     * Usage:
     *  - stationtable table: left cells from second row to last row
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "table-cell");
    xml.writeAttribute("style:name", "stationtable.L2");

    xml.writeStartElement("style:table-cell-properties");
    xml.writeAttribute("fo:padding", "0.097cm");
    xml.writeAttribute("fo:border-left", "0.05pt solid #000000");
    xml.writeAttribute("fo:border-right", "0.05pt solid #000000");
    xml.writeAttribute("fo:border-top", "none");
    xml.writeAttribute("fo:border-bottom", "0.05pt solid #000000");
    xml.writeEndElement(); //style:table-cell-properties
    xml.writeEndElement(); //style

    /* Style P2
     * type:        paragraph
     * text-align:  center
     * font-size:   13pt
     * font-weight: bold
     *
     * Usages:
     * - stationtable table: header cells
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:name", "P2");

    xml.writeStartElement("style:paragraph-properties");
    xml.writeAttribute("fo:text-align", "center");
    xml.writeAttribute("style:justify-single-word", "false");
    xml.writeEndElement(); //style:paragraph-properties

    xml.writeStartElement("style:text-properties");
    xml.writeAttribute("fo:font-size", "13pt");
    xml.writeAttribute("fo:font-weight", "bold");
    xml.writeEndElement(); //style:text-properties

    xml.writeEndElement(); //style:style

    /* Style P3
     * type:        paragraph
     * text-align:  center
     * font-size:   10pt
     *
     * Usages:
     * - stationtable table: content cells
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:name", "P3");

    xml.writeStartElement("style:paragraph-properties");
    xml.writeAttribute("fo:text-align", "center");
    xml.writeAttribute("style:justify-single-word", "false");
    xml.writeEndElement(); //style:paragraph-properties

    xml.writeStartElement("style:text-properties");
    xml.writeAttribute("fo:font-size", "10pt");
    xml.writeEndElement(); //style:text-properties

    xml.writeEndElement(); //style:style

    //P5 - Bold, Italics, for Transits
    /* Style P5
     * type:        paragraph
     * text-align:  center
     * font-size:   12pt
     * font-weight: bold
     * font-style:  italic
     *
     * Usages:
     * - stationtable table: Arrival for transit jobs (Normal stops have bold Arrival, transits have Bold + Italic)
     */
    xml.writeStartElement("style:style");
    xml.writeAttribute("style:family", "paragraph");
    xml.writeAttribute("style:name", "P5");

    xml.writeStartElement("style:paragraph-properties");
    xml.writeAttribute("fo:text-align", "center");
    xml.writeAttribute("style:justify-single-word", "false");
    xml.writeEndElement(); //style:paragraph-properties

    xml.writeStartElement("style:text-properties");
    xml.writeAttribute("fo:font-size", "12pt");
    xml.writeAttribute("fo:font-weight", "bold");
    xml.writeAttribute("fo:font-style", "italic");
    xml.writeEndElement(); //style:text-properties

    xml.writeEndElement(); //style:style
}

void StationWriter::writeStation(QXmlStreamWriter &xml, db_id stationId, QString *stNameOut)
{
    QMap<QTime, Stop> stops; //Order by Departure ASC

    query q_getStName(mDb, "SELECT name,short_name FROM stations WHERE id=?");

    query q_getPrevStop(mDb, "SELECT id, station_id, MAX(departure) FROM stops"
                             " WHERE job_id=? AND departure<?");
    query q_getNextStop(mDb, "SELECT id, station_id, MIN(arrival) FROM stops"
                             " WHERE job_id=? AND arrival>?");

    QString stationName;
    QString shortName;
    q_getStName.bind(1, stationId);
    if(q_getStName.step() == SQLITE_ROW)
    {
        auto r = q_getStName.getRows();
        stationName = r.get<QString>(0);
        shortName = r.get<QString>(1);
    }
    q_getStName.reset();
    if(stNameOut)
        *stNameOut = stationName;

    //Title
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "P1");
    xml.writeCharacters(Odt::tr("Station: %1%2")
                            .arg(stationName)
                            .arg(shortName.isEmpty() ?
                                     QString() :
                                     Odt::tr(" (%1)").arg(shortName)));
    xml.writeEndElement();

    //Vertical space
    xml.writeStartElement("text:p");
    xml.writeAttribute("text:style-name", "P1");
    xml.writeEndElement();

    //stationtable
    xml.writeStartElement("table:table");
    //xml.writeAttribute("table:name", "stationtable");
    xml.writeAttribute("table:style-name", "stationtable");

    xml.writeEmptyElement("table:table-column"); //Arrival
    xml.writeAttribute("table:style-name", "stationtable.A");

    xml.writeEmptyElement("table:table-column"); //Departure
    xml.writeAttribute("table:style-name", "stationtable.B");

    xml.writeEmptyElement("table:table-column"); //Job N
    xml.writeAttribute("table:style-name", "stationtable.C");

    xml.writeEmptyElement("table:table-column"); //Platform (Bin)
    xml.writeAttribute("table:style-name", "stationtable.D");

    xml.writeEmptyElement("table:table-column"); //Comes From
    xml.writeAttribute("table:style-name", "stationtable.E");

    xml.writeEmptyElement("table:table-column"); //Goes To
    xml.writeAttribute("table:style-name", "stationtable.F");

    xml.writeEmptyElement("table:table-column"); //Rollingstock
    xml.writeAttribute("table:style-name", "stationtable.G");

    xml.writeEmptyElement("table:table-column"); //Crossings
    xml.writeAttribute("table:style-name", "stationtable.H");

    xml.writeEmptyElement("table:table-column"); //Passings
    xml.writeAttribute("table:style-name", "stationtable.I");

    xml.writeEmptyElement("table:table-column"); //Notes
    xml.writeAttribute("table:style-name", "stationtable.L");

    //Row 1 (Heading)
    xml.writeStartElement("table:table-header-rows");
    xml.writeStartElement("table:table-row");

    writeCell(xml, "stationtable.A1", "P2", Odt::tr("Arrival"));
    writeCell(xml, "stationtable.A1", "P2", Odt::tr("Departure"));
    writeCell(xml, "stationtable.A1", "P2", Odt::tr("Job N"));
    writeCell(xml, "stationtable.A1", "P2", Odt::tr("Platf"));
    writeCell(xml, "stationtable.A1", "P2", Odt::tr("From"));
    writeCell(xml, "stationtable.A1", "P2", Odt::tr("To"));
    writeCell(xml, "stationtable.A1", "P2", Odt::tr("Rollingstock"));
    writeCell(xml, "stationtable.A1", "P2", Odt::tr("Crossings"));
    writeCell(xml, "stationtable.A1", "P2", Odt::tr("Passings"));
    writeCell(xml, "stationtable.L1", "P2", Odt::tr("Notes")); //Description

    xml.writeEndElement(); //end of row
    xml.writeEndElement(); //header section

    //Stops
    q_getJobsByStation.bind(1, stationId);
    for(auto r : q_getJobsByStation)
    {
        db_id stopId = r.get<db_id>(0);
        Stop stop;
        stop.jobId = r.get<db_id>(1);
        stop.jobCat = JobCategory(r.get<int>(2));
        stop.arrival = r.get<QTime>(3);
        stop.departure = r.get<QTime>(4);
        const int stopType = r.get<int>(5);
        stop.description = r.get<QString>(6);

        stop.platform = r.get<QString>(7);
        if(stop.platform.isEmpty())
            stop.platform = r.get<QString>(8); //Use out gate to get track name

        const bool isTransit = stopType == 1;

        //BIG TODO: if this is First or Last stop of this job
        //then it shouldn't be duplicated in 2 rows

        //Comes from station
        q_getPrevStop.bind(1, stop.jobId);
        q_getPrevStop.bind(2, stop.arrival);
        if(q_getPrevStop.step() == SQLITE_ROW)
        {
            db_id prevStId = q_getPrevStop.getRows().get<db_id>(1);
            q_getStName.bind(1, prevStId);
            if(q_getStName.step() == SQLITE_ROW)
            {
                stop.prevSt = q_getStName.getRows().get<QString>(0);
            }
            q_getStName.reset();
        }
        q_getPrevStop.reset();

        //Goes to station
        q_getNextStop.bind(1, stop.jobId);
        q_getNextStop.bind(2, stop.arrival);
        if(q_getNextStop.step() == SQLITE_ROW)
        {
            db_id nextStId = q_getNextStop.getRows().get<db_id>(1);
            q_getStName.bind(1, nextStId);
            if(q_getStName.step() == SQLITE_ROW)
            {
                stop.nextSt = q_getStName.getRows().get<QString>(0);
            }
            q_getStName.reset();
        }
        q_getNextStop.reset();

        for(auto s = stops.begin(); s != stops.end(); /*nothing because of erase*/)
        {
            //If 's' departs after 'stop' arrives then skip 's' for now
            if(s->departure >= stop.arrival)
            {
                ++s;
                continue;
            }

            //'s' departs before 'stop' arrives so we
            //insert it again to remind station master (IT: capostazione)
            insertStop(xml, s.value(), false, false);
            xml.writeEndElement(); //table-row

            //Then remove from the list otherwise it gets inserted infinite times
            s = stops.erase(s);
        }

        //Fill with basic data
        insertStop(xml, stop, true, isTransit);

        //First time this stop is written, fill with other data

        //Rollingstock
        sqlite3_stmt *stmt = q_getStopCouplings.stmt();
        writeCellListStart(xml, "stationtable.A2", "P3");

        //Coupled rollingstock
        bool firstCoupRow = true;
        q_getStopCouplings.bind(1, stopId);
        q_getStopCouplings.bind(2, int(RsOp::Coupled));
        for(auto coup : q_getStopCouplings)
        {
            //db_id rsId = coup.get<db_id>(0);

            int number = coup.get<int>(1);
            int modelNameLen = sqlite3_column_bytes(stmt, 2);
            const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 2));

            int modelSuffixLen = sqlite3_column_bytes(stmt, 3);
            const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 3));
            RsType type = RsType(sqlite3_column_int(stmt, 4));

            const QString rsName = rs_utils::formatNameRef(modelName, modelNameLen,
                                                           number,
                                                           modelSuffix, modelSuffixLen,
                                                           type);

            if(firstCoupRow)
            {
                firstCoupRow = false;
                //Use bold font
                xml.writeStartElement("text:span");
                xml.writeAttribute("text:style-name", "T1");
                xml.writeCharacters(Odt::tr(Odt::CoupledAbbr));
                xml.writeEndElement(); //test:span
            }

            xml.writeEmptyElement("text:line-break");
            xml.writeCharacters(rsName);
        }
        q_getStopCouplings.reset();

        //Unoupled rollingstock
        bool firstUncoupRow = true;
        q_getStopCouplings.bind(1, stopId);
        q_getStopCouplings.bind(2, int(RsOp::Uncoupled));
        for(auto coup : q_getStopCouplings)
        {
            //db_id rsId = coup.get<db_id>(0);

            int number = coup.get<int>(1);
            int modelNameLen = sqlite3_column_bytes(stmt, 2);
            const char *modelName = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 2));

            int modelSuffixLen = sqlite3_column_bytes(stmt, 3);
            const char *modelSuffix = reinterpret_cast<char const*>(sqlite3_column_text(stmt, 3));
            RsType type = RsType(sqlite3_column_int(stmt, 4));

            const QString rsName = rs_utils::formatNameRef(modelName, modelNameLen,
                                                           number,
                                                           modelSuffix, modelSuffixLen,
                                                           type);

            if(firstUncoupRow)
            {
                if(!firstCoupRow) //Not first row, there were coupled rs
                    xml.writeEmptyElement("text:line-break"); //Separate from coupled
                firstUncoupRow = false;

                //Use bold font
                xml.writeStartElement("text:span");
                xml.writeAttribute("text:style-name", "T1");
                xml.writeCharacters(Odt::tr(Odt::UncoupledAbbr));
                xml.writeEndElement(); //test:span
            }

            xml.writeEmptyElement("text:line-break");
            xml.writeCharacters(rsName);
        }
        q_getStopCouplings.reset();
        writeCellListEnd(xml);

        //Crossings, Passings
        QVector<JobEntry> passings;
        JobStopDirectionHelper dirHelper(mDb);
        utils::Side myDirection = dirHelper.getStopOutSide(stopId);

        q_selectPassings.bind(1, stationId);
        q_selectPassings.bind(2, stop.arrival);
        q_selectPassings.bind(3, stop.departure);
        q_selectPassings.bind(4, stop.jobId);
        firstCoupRow = true;

        //Incroci
        writeCellListStart(xml, "stationtable.A2", "P3");
        for(auto pass : q_selectPassings)
        {
            db_id otherStopId = pass.get<db_id>(0);
            db_id otherJobId = pass.get<db_id>(1);
            JobCategory otherJobCat = JobCategory(pass.get<int>(2));

            //QTime otherArr = pass.get<QTime>(3);
            //QTime otherDep = pass.get<QTime>(4);

            utils::Side otherDir = dirHelper.getStopOutSide(otherStopId);

            if(myDirection == otherDir)
                passings.append({otherJobId, otherJobCat});
            else
            {
                if(firstCoupRow)
                    firstCoupRow = false;
                else
                    xml.writeEmptyElement("text:line-break");
                xml.writeCharacters(JobCategoryName::jobName(otherJobId, otherJobCat));
            }
        }
        q_selectPassings.reset();
        writeCellListEnd(xml);

        //Passings
        firstCoupRow = true;
        writeCellListStart(xml, "stationtable.A2", "P3");
        for(auto entry : passings)
        {
            if(firstCoupRow)
                firstCoupRow = false;
            else
                xml.writeEmptyElement("text:line-break");
            xml.writeCharacters(JobCategoryName::jobName(entry.jobId, entry.category));
        }
        writeCellListEnd(xml);

        //Description, Notes
        writeCellListStart(xml, "stationtable.L2", "P3");
        if(isTransit)
        {
            xml.writeCharacters(Odt::tr("Transit"));
        }
        if(!stop.description.isEmpty())
        {
            if(isTransit) //go to new line after 'Transit' word
                xml.writeEmptyElement("text:line-break");

            //Split in lines
            int lastIdx = 0;
            while(true)
            {
                int idx = stop.description.indexOf('\n', lastIdx);
                QString line = stop.description.mid(lastIdx, idx == -1 ? idx : idx - lastIdx);
                xml.writeCharacters(line.simplified());
                if(idx < 0)
                    break; //Last line
                lastIdx = idx + 1;
                xml.writeEmptyElement("text:line-break");
            }
        }
        writeCellListEnd(xml);

        xml.writeEndElement(); //table-row

        if(!isTransit)
        {
            //If it is a normal stop (not a transit)
            //insert two rows: one for arrival and another
            //that reminds the station master (capostazione)
            //the departure of that train
            //In order to achieve this we put the stop in a list
            //and write it again before another arrival (see above)
            stops.insert(stop.departure, stop);
        }
    }
    q_getJobsByStation.reset();

    for(const Stop& s : stops)
    {
        insertStop(xml, s, false, false);
        xml.writeEndElement(); //table-row
    }

    xml.writeEndElement(); //stationtable end
}
