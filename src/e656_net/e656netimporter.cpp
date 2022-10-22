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

        if(xml.name() != QLatin1String("tbody") && xml.attributes().value(QLatin1String("class")) == QLatin1String("list-table"))
        {
            continue; //Not our element, skip
        }

        break;
    }

    return xml.hasError();
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


}
