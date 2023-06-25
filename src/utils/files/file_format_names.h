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

#ifndef FILE_FORMAT_NAMES_H
#define FILE_FORMAT_NAMES_H

#include <QCoreApplication>

class FileFormats
{
    Q_DECLARE_TR_FUNCTIONS(FileFormats)

public:
    // BIG TODO: maybe use mimetypes so we automatically get the name from system
    // TODO: add custom extension that in future will be registered with an Icon (*.ttt)
    static constexpr const char *allFiles = QT_TRANSLATE_NOOP("FileFormats", "All Files (*.*)");
    static constexpr const char *odsFormat =
      QT_TRANSLATE_NOOP("FileFormats", "OpenDocument Sheet (*.ods)");
    static constexpr const char *odtFormat =
      QT_TRANSLATE_NOOP("FileFormats", "OpenDocument Text (*.odt)");
    static constexpr const char *tttFormat =
      QT_TRANSLATE_NOOP("FileFormats", "MRTPlanner Session (*.ttt)");
    static constexpr const char *sqliteFormat =
      QT_TRANSLATE_NOOP("FileFormats", "SQLite 3 Database (*.db *.sqlite *.sqlite3 *.db3)");
    static constexpr const char *svgFile =
      QT_TRANSLATE_NOOP("FileFormats", "SVG vectorial image (*.svg)");
    static constexpr const char *xmlFile = QT_TRANSLATE_NOOP("FileFormats", "XML (*.xml)");
    static constexpr const char *pdfFile =
      QT_TRANSLATE_NOOP("FileFormats", "Portable Document Format (*.pdf)");
};

#endif // FILE_FORMAT_NAMES_H
