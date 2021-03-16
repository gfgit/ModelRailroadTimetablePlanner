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

    //Returns a 'path + file name' where you must save the image
    QString addImage(const QString& name, const QString& mediaType);

    inline void setTitle(const QString& title) { documentTitle = title; }

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
    //pair: fileName, mediaType
    QList<QPair<QString, QString>> imageList;
};

#endif // ODTDOCUMENT_H
