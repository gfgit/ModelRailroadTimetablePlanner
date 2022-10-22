#include "e656netimporter.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QXmlStreamReader>

#include <QDebug>

bool jumpToJobStart(QXmlStreamReader &xml)
{
    while(!xml.atEnd())
    {
        if(xml.readNext() != QXmlStreamReader::StartElement)
        {
            continue; //Skip previous elements
        }

        qDebug() << xml.name();

        if(xml.name() != QLatin1String("tbody") || xml.attributes().value(QLatin1String("class")) != QLatin1String("list-table"))
        {
            continue; //Not our element, skip
        }

        break;
    }

    return !xml.hasError();
}

bool readJobTable(QXmlStreamReader &xml)
{
    qDebug() << xml.name();
    while(!xml.atEnd())
    {
        //Read row element <tr>
        xml.readNextStartElement();
        qDebug() << xml.name();

        xml.readNextStartElement(); //Station Name

        qDebug() << xml.name();
        QString stName = xml.readElementText();
        xml.skipCurrentElement();

        xml.readNextStartElement(); //Arrival
        qDebug() << xml.name();
        QString arr = xml.readElementText();
        xml.skipCurrentElement();

        xml.readNextStartElement(); //Departure
        qDebug() << xml.name();
        QString dep = xml.readElementText();
        xml.skipCurrentElement();

        xml.readNextStartElement(); //Platform
        qDebug() << xml.name();
        QString platf = xml.readElementText(QXmlStreamReader::IncludeChildElements);
        xml.skipCurrentElement();

        qDebug() << "Station:" << stName << arr << dep << platf;

        xml.skipCurrentElement(); //End row
    }

    return !xml.hasError();
}

E656NetImporter::E656NetImporter(sqlite3pp::database &db, QObject *parent) :
    QObject(parent),
    mDb(db)
{
    manager = new QNetworkAccessManager(this);
}

bool E656NetImporter::startImportJob(const QUrl &url, bool overwriteExisting)
{
    if(!overwriteExisting)
    {
        //TODO: check existing
    }

    QNetworkRequest req(url);
    QNetworkReply *reply = manager->get(req);
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply]()
        {
            emit errorOccurred(tr("Network Error:\n"
                                  "%1\n"
                                  "%2")
                                   .arg(reply->url().toString(),
                                       reply->errorString()));
            reply->deleteLater();
        });

    connect(reply, &QNetworkReply::finished, this, [this, reply]()
        {
            doImportJob(reply);
        });

    return true;
}

void E656NetImporter::doImportJob(QNetworkReply *reply)
{
    qDebug() << "IMPORTING JOB:" << reply->url();

    reply->deleteLater();

    if(reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error:" << reply->errorString();

        emit errorOccurred(tr("Network Error:\n"
                              "%1\n"
                              "%2")
                               .arg(reply->url().toString(),
                                   reply->errorString()));
        return;
    }

    QXmlStreamReader xml(reply);
    if(!jumpToJobStart(xml))
    {
        qDebug() << "XML ERROR:" << xml.errorString();
        return;
    }

    if(!readJobTable(xml))
    {
        qDebug() << "XML ERROR:" << xml.errorString();
        return;
    }
}
