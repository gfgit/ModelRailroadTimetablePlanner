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

#ifndef METADATAMANAGER_H
#define METADATAMANAGER_H

#include <QString>
#include <QByteArray>

namespace sqlite3pp {
class database;
}

namespace MetaDataKey
{
enum Result
{
    ValueFound = 0,
    ValueIsNull,
    ValueNotFound,
    NoMetaDataTable, //Format is too old, 'metadata' table is not present

    NResults
};

//BEGING Key constants TODO: maybe make static or extern to avoid duplication

//Database
constexpr char FormatVersionKey[] = "format_version"; //INTEGER: version, NOTE: FormatVersion is aleady used by info.h constants
constexpr char ApplicationString[] = "application_str"; //STRING: application version string 'maj.min.patch'

//Meeting
constexpr char MeetingShowDates[] = "meeting_show_dates"; //INTEGER: 1 shows dates, 0 hides them
constexpr char MeetingStartDate[] = "meeting_start_date"; //INTEGER: Start date in Julian Day integer
constexpr char MeetingEndDate[]   = "meeting_end_date";   //INTEGER: End date in Juliand Day integer
constexpr char MeetingLocation[]  = "meeting_location";   //STRING: city name
constexpr char MeetingDescription[] = "meeting_descr";    //STRING: brief description of the meeting
constexpr char MeetingHostAssociation[] = "meeting_host"; //STRING: name of association that is hosting the meeting
constexpr char MeetingLogoPicture[] = "meeting_logo";     //BLOB: PNG alpha image, usually hosting association logo

//ODT Export Sheet
constexpr char SheetHeaderText[] = "sheet_header"; //STRING: sheet header text
constexpr char SheetFooterText[] = "sheet_footer"; //STRING: sheet footer text

//Jobs
#define METADATA_MAKE_RS_KEY(category) ("job_default_stop_" ## #category)

//END Key constants
}

class MetaDataManager
{
public:
    MetaDataManager(sqlite3pp::database &db);

    struct Key
    {
        template<int N>
        constexpr inline Key(const char (&val)[N]) :str(val), len(N - 1) {}

        const char *str;
        const int len;
    };

    MetaDataKey::Result hasKey(const Key& key);

    MetaDataKey::Result getInt64(qint64 &out, const Key& key);
    MetaDataKey::Result setInt64(qint64 in, bool setToNull, const Key &key);

    MetaDataKey::Result getString(QString &out, const Key &key);
    MetaDataKey::Result setString(const QString &in, bool setToNull, const Key &key);

private:
    sqlite3pp::database &mDb;
};

#endif // METADATAMANAGER_H
