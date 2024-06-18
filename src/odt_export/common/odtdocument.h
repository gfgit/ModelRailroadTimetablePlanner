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

#ifndef ODTDOCUMENT_H
#define ODTDOCUMENT_H

#include <QTemporaryDir>
#include <QFile>
#include <QXmlStreamWriter>

class OdtDocument
{
public:
    OdtDocument();

    bool saveTo(const QString &fileName);

    bool initDocument();
    void startBody();
    void endDocument();

    // Returns a 'path + file name' where you must save the image
    QString addImage(const QString &name, const QString &mediaType);

    inline void setTitle(const QString &title)
    {
        documentTitle = title;
    }

public:
    QTemporaryDir dir;
    QFile content;
    QFile styles;

    QXmlStreamWriter contentXml;
    QXmlStreamWriter stylesXml;

private:
    void writeStartDoc(QXmlStreamWriter &xml);
    void saveMimetypeFile(const QString &path);
    void saveManifest(const QString &path);
    void saveMeta(const QString &path);
    void writeFileEntry(QXmlStreamWriter &xml, const QString &fullPath, const QString &mediaType);

private:
    QString documentTitle;
    // pair: fileName, mediaType
    QList<std::pair<QString, QString>> imageList;
};

#endif // ODTDOCUMENT_H
