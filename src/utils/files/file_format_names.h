#ifndef FILE_FORMAT_NAMES_H
#define FILE_FORMAT_NAMES_H

#include <QCoreApplication>

class FileFormats
{
    Q_DECLARE_TR_FUNCTIONS(FileFormats)

public:
    //BIG TODO: maybe use mimetypes so we automatically get the name from system
    //TODO: add custom extension that in future will be registered with an Icon (*.ttt)
    static constexpr const char *allFiles = QT_TRANSLATE_NOOP("FileFormats", "All Files (*.*)");
    static constexpr const char *odsFormat = QT_TRANSLATE_NOOP("FileFormats", "OpenDocument Sheet (*.ods)");
    static constexpr const char *odtFormat = QT_TRANSLATE_NOOP("FileFormats", "OpenDocument Text (*.odt)");
    static constexpr const char *tttFormat = QT_TRANSLATE_NOOP("FileFormats", "MRTPlanner Session (*.ttt)");
    static constexpr const char *sqliteFormat = QT_TRANSLATE_NOOP("FileFormats", "SQLite 3 Database (*.db *.sqlite *.sqlite3 *.db3)");
    static constexpr const char *svgFile = QT_TRANSLATE_NOOP("FileFormats", "SVG vectorial image (*.svg)");
    static constexpr const char *xmlFile = QT_TRANSLATE_NOOP("FileFormats", "XML (*.xml)");
    static constexpr const char *pdfFile = QT_TRANSLATE_NOOP("FileFormats", "Portable Document Format (*.pdf)");
};

#endif // FILE_FORMAT_NAMES_H
